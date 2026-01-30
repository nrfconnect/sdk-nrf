#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Main entry point for the script.
'''

from pathlib import Path

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
from args import args, init_args
from common import SbomException, dbg_time
from data_structure import Data
from west import log

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


def _run_pipeline():
    data = Data()

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

    output_pre_process.pre_process(data)

    for generator_name, generator in generators.items():
        output_file = args.__dict__[f'output_{generator_name}']
        if output_file is not None:
            t = dbg_time(f'GENERATOR: {generator_name}')
            output_template.generate(data, output_file, Path(__file__).parent / generator)
            log.dbg(f'GENERATOR: Done in {t}s')


def main():
    '''Main entry function for the script.'''
    try:
        init_args(detectors)

        def domain_output(output_path: 'str|Path|None', domain: str, default_name: str) -> 'str|None':
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

        if args.build_dir is not None:
            sysbuild_entries = []
            other_entries = []
            for entry in args.build_dir:
                domains = input_build.get_sysbuild_domains(Path(entry[0]))
                if domains:
                    sysbuild_entries.append((entry, domains))
                else:
                    other_entries.append(entry)

            if sysbuild_entries:
                if len(sysbuild_entries) > 1:
                    raise SbomException('Only one sysbuild build directory is supported')

                defaults = {
                    'html': 'sbom_report_{domain}.html',
                    'spdx': 'sbom_{domain}.spdx',
                    'cache_database': 'sbom_cache_{domain}.json',
                }
                base_outputs = {f'output_{k}': args.__dict__.get(f'output_{k}')
                                for k in generators}
                entry, domains = sysbuild_entries[0]
                for domain_name, domain_dir in domains:
                    log.inf(f'Generating SBOM for sysbuild domain "{domain_name}"')
                    domain_entry = [str(domain_dir)] + entry[1:]
                    args.build_dir = other_entries + [domain_entry]
                    for gen in generators:
                        key = f'output_{gen}'
                        args.__dict__[key] = domain_output(base_outputs[key],
                                                           domain_name,
                                                           defaults[gen])
                    _run_pipeline()
                return

        _run_pipeline()
    except SbomException as e:
        log.die(str(e), exit_code=1)


if __name__ == '__main__':
    main()
