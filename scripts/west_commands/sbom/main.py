#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Main entry point for the script.
'''

import os
from datetime import datetime, timezone
from pathlib import Path
from typing import TYPE_CHECKING

import cache_database_detector
import external_file_detector
import file_input
import full_text_detector
import git_info_detector
import input_build
import input_post_process
import output_pre_process
import output_template
import scancode_toolkit_detector
import spdx_tag_detector
from args import SPDX_VERSION_3_0, args, init_args
from common import SbomException, dbg_time
from data_structure import Data
from west import log

if TYPE_CHECKING:
    from sign_spdx import SpdxSigner

inputs = {
    'input-build': input_build.generate_input,
    'file-input': file_input.generate_input,
}

detectors = {
    'spdx-tag': spdx_tag_detector.detect,
    'git-info': git_info_detector.detect,
    'full-text': full_text_detector.detect,
    'scancode-toolkit': scancode_toolkit_detector.detect,
    'cache-database': cache_database_detector.detect,
    'external-file': external_file_detector.detect,
}

generators = {
    'html': 'templates/report.html.jinja',
    'spdx': 'templates/raport.spdx.jinja',
    'cache_database': 'templates/cache.database.jinja',
}


def _written_paths(generator_name: str, output: 'str|Path', signer: 'SpdxSigner|None') -> list[str]:
    '''Return the output file and, for a signed SPDX report, its signature bundle.'''
    paths = [str(output)]
    if generator_name == 'spdx' and signer is not None:
        paths.append(str(output) + '.sigstore.json')
    return paths


def _same_file(first: str, second: str) -> bool:
    '''Return True when both paths name the same file, following symlinks and hard links.'''
    if os.path.normcase(os.path.realpath(first)) == os.path.normcase(os.path.realpath(second)):
        return True
    return os.path.exists(first) and os.path.exists(second) and os.path.samefile(first, second)


def _reject_output_collisions(runs: 'list[dict[str, str|None]]', signer: 'SpdxSigner|None'):
    '''Stop before anything is written if two outputs, a signature bundle or the exported public
    key would be written to the same file, including through symlinks and hard links.
    Each item of runs maps generator names to the output paths of one pipeline run (one sysbuild
    domain), so that collisions between domains are found too.'''
    targets = [
        path
        for outputs in runs
        for name, output in outputs.items()
        if output is not None
        for path in _written_paths(name, output, signer)
    ]
    if signer is not None and args.public_key_out is not None:
        targets.append(str(args.public_key_out))
        # The exported key must not replace a file that the command still has to read.
        inputs = [*(args.input_list_file or []), args.input_cache_database]
        for path in filter(None, inputs):
            if _same_file(args.public_key_out, path):
                raise SbomException(f'--public-key-out {args.public_key_out} is an input file')
    for i, first in enumerate(targets):
        for second in targets[i + 1 :]:
            if _same_file(first, second):
                raise SbomException(f'{first} and {second} would be written to the same file')


def _prepare_outputs(runs: 'list[dict[str, str|None]]', signer: 'SpdxSigner|None'):
    '''Validate every path that will be written and only then export the public key, so that a
    rejected command has not modified any file.'''
    _reject_output_collisions(runs, signer)
    if signer is None:
        return
    if args.debug_build_input_cache is not None:
        signer.check_not_key(args.debug_build_input_cache)
    if args.public_key_out is not None:
        signer.export_public_key(args.public_key_out)


def _generate_output(
    data: Data,
    generator_name: str,
    generator: str,
    output_file: 'str|Path',
    signer: 'SpdxSigner|None',
):
    '''Write one output file and, for an SPDX report, its signature bundle.'''
    sign = generator_name == 'spdx' and signer is not None
    if signer is not None:
        signer.check_not_key(output_file)
    if sign:
        # A failing serializer can leave a partial report; drop its old signature.
        signer.remove_bundle(output_file)
    if generator_name == 'spdx' and args.spdx_version == SPDX_VERSION_3_0:
        # Imported here so spdx-python-model is only required for SPDX 3.0 output.
        import spdx3_serializer

        spdx3_serializer.generate(data, output_file)
    else:
        output_template.generate(data, output_file, Path(__file__).parent / generator)
    if sign:
        signer.sign(output_file)


def _run_pipeline(domain: 'str|None' = None, signer: 'SpdxSigner|None' = None):
    data = Data()
    data.domain = domain

    for input_name, input in inputs.items():
        t = dbg_time(f'INPUT: {input_name}')
        input(data)
        log.dbg(f'INPUT: Done in {t}s')

    input_post_process.post_process(data)

    for detector_name in args.license_detectors:
        func = detectors[detector_name]
        optional = detector_name in args.optional_license_detectors
        t = dbg_time(f'DETECTOR: {detector_name}')
        func(data, optional)
        log.dbg(f'DETECTOR: Done in {t}s')

    output_pre_process.pre_process(data, built_date=_compute_built_date())

    for generator_name, generator in generators.items():
        output_file = args.__dict__[f'output_{generator_name}']
        if output_file is not None:
            t = dbg_time(f'GENERATOR: {generator_name}')
            _generate_output(data, generator_name, generator, output_file, signer)
            log.dbg(f'GENERATOR: Done in {t}s')


def _compute_built_date() -> 'str|None':
    '''Newest mtime across *.map and zephyr/runners.yaml as UTC ISO 8601,
    or None when no artifact exists.'''
    if not args.build_dir:
        return None
    mtimes = []
    for entry in args.build_dir:
        build_dir = Path(entry[0])
        targets = entry[1:] or [input_build.DEFAULT_TARGET]
        for target_with_map in targets:
            target_name, separator, map_name = target_with_map.partition(':')
            if separator:
                map_file = build_dir / map_name
            else:
                map_file = (build_dir / target_name).with_suffix('.map')
            if map_file.exists():
                mtimes.append(map_file.stat().st_mtime)
        runners_yaml = build_dir / 'zephyr' / 'runners.yaml'
        if runners_yaml.exists():
            mtimes.append(runners_yaml.stat().st_mtime)
    if not mtimes:
        return None
    return (
        datetime.fromtimestamp(max(mtimes), tz=timezone.utc)
        .isoformat(timespec='seconds')
        .replace('+00:00', 'Z')
    )


def _target_maps_present(build_dir: Path, targets: list[str]) -> bool:
    '''
    Return True if map file is present in build directory.
    '''
    resolved_targets = targets or [input_build.DEFAULT_TARGET]
    for target_with_map in resolved_targets:
        target_name, separator, map_name = target_with_map.partition(':')
        if separator:
            map_file = build_dir / map_name
        else:
            map_file = (build_dir / target_name).with_suffix('.map')
        if not map_file.exists():
            return False
    return True


def _domain_inputs_present(build_dir: Path, targets: list[str]) -> bool:
    '''
    Return True if the domain can be analyzed either via map files or
    via a hex files in runners.yaml.
    '''
    if _target_maps_present(build_dir, targets):
        return True
    if len(targets) <= 1 and (len(targets) == 0 or targets[0] == input_build.DEFAULT_TARGET):
        return input_build.get_runner_artifact(build_dir) is not None
    return False


def _import_signing():
    '''Import the signing module only when signing or verifying, so that the other commands work
    without the packages it needs.'''
    try:
        import sign_spdx
    except ImportError as ex:
        raise SbomException(
            f'Signing and verifying SPDX reports requires a missing Python package ({ex}).\n'
            'Install the SBOM requirements with:\n'
            '  pip3 install -r scripts/requirements-west-ncs-sbom.txt'
        ) from ex
    return sign_spdx


def domain_output(output_path: 'str|Path|None', domain: str, default_name: str) -> 'str|None':
    '''Return the output path of one sysbuild domain.'''
    if output_path is None:
        return None
    safe_domain = domain.strip().replace('/', '_').replace('\\', '_') or 'domain'
    output_str = str(output_path)
    if '{domain}' in output_str:
        return output_str.replace('{domain}', safe_domain)
    path = Path(output_str)
    if output_str.endswith(('/', '\\')) or (path.exists() and path.is_dir()):
        return str(path / default_name.format(domain=safe_domain))
    if path.suffix:
        return str(path.with_name(f'{path.stem}_{safe_domain}{path.suffix}'))
    return str(path.with_name(f'{path.name}_{safe_domain}'))


def _setup_signing() -> 'tuple[SpdxSigner|None, bool]':
    '''Verify reports or load the signing key. Return the signer and whether the command is done.'''
    sign_spdx = _import_signing()
    if args.verify_spdx is not None:
        sign_spdx.verify_reports(args.verify_spdx, args.verify_key)
        return None, True
    return sign_spdx.SpdxSigner(args.sign_key), False


def _find_sysbuild() -> 'tuple[list, list, list]|None':
    '''Return the (entry, domains, other build entries) of the sysbuild build directory, if any.'''
    sysbuild_entries = []
    other_entries = []
    for entry in args.build_dir or []:
        domains = input_build.get_sysbuild_domains(Path(entry[0]))
        if domains:
            sysbuild_entries.append((entry, domains))
        else:
            other_entries.append(entry)
    if len(sysbuild_entries) > 1:
        raise SbomException('Only one sysbuild build directory is supported')
    if not sysbuild_entries:
        return None
    return (*sysbuild_entries[0], other_entries)


def _run_sysbuild(entry: list, domains: list, other_entries: list, signer: 'SpdxSigner|None'):
    '''Generate one report set per sysbuild domain, after checking that they cannot collide.'''
    defaults = {
        'html': 'sbom_report_{domain}.html',
        'spdx': (
            'sbom_{domain}.jsonld'
            if args.spdx_version == SPDX_VERSION_3_0
            else 'sbom_{domain}.spdx'
        ),
        'cache_database': 'sbom_cache_{domain}.json',
    }
    base_outputs = {gen: args.__dict__.get(f'output_{gen}') for gen in generators}
    selected = []
    for domain_name, domain_dir in domains:
        if _domain_inputs_present(domain_dir, entry[1:]):
            selected.append((domain_name, domain_dir))
        else:
            log.wrn(f'Skipping "{domain_name}" because no map file or hex was found"{domain_dir}"')
    if not selected:
        raise SbomException('No sysbuild domains contain map or hex files')

    # One plan per selected entry, so that domains with the same name collide too.
    plans = [
        {gen: domain_output(base_outputs[gen], name, defaults[gen]) for gen in generators}
        for name, _ in selected
    ]
    _prepare_outputs(plans, signer)

    for (domain_name, domain_dir), plan in zip(selected, plans, strict=True):
        log.inf(f'Generating SBOM for sysbuild domain "{domain_name}"')
        args.build_dir = other_entries + [[str(domain_dir)] + entry[1:]]
        for gen in generators:
            args.__dict__[f'output_{gen}'] = plan[gen]
        _run_pipeline(domain_name, signer)


def main():
    '''Main entry function for the script.'''
    try:
        init_args(detectors)
        signer = None
        if args.verify_spdx is not None or args.sign_key is not None:
            signer, finished = _setup_signing()
            if finished:
                return

        if signer is not None and args.output_spdx is None:
            # Only the public key was requested.
            _prepare_outputs([], signer)
            return

        sysbuild = _find_sysbuild()
        if sysbuild is not None:
            _run_sysbuild(*sysbuild, signer)
            return

        _prepare_outputs([{gen: args.__dict__.get(f'output_{gen}') for gen in generators}], signer)
        _run_pipeline(signer=signer)
    except SbomException as e:
        log.die(str(e), exit_code=1)


if __name__ == '__main__':
    main()
