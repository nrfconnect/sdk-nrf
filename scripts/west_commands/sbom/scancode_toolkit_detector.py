#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Implementation of a detector based on an external tool - scancode-toolkit.
For more details see: https://scancode-toolkit.readthedocs.io/en/stable/
'''

import concurrent.futures
import json
import os
import re
import shutil
from contextlib import suppress
from tempfile import NamedTemporaryFile
from time import sleep

from args import args
from common import SbomException, command_execute
from data_structure import Data, FileInfo, License
from license_utils import is_spdx_license
from west import log

SCANCODE_DEFAULT_PARALLEL_WORKERS = 4
SCANCODE_RUN_RETRIES = 2


def check_scancode():
    '''Checks if "scancode --version" works correctly. If not, raises exception with information
    for user.'''
    if shutil.which(args.scancode) is None:
        raise SbomException(f'Cannot find scancode executable "{args.scancode}".\n'
            'Install the SBOM requirements with:\n'
            '  pip3 install -r scripts/requirements-west-ncs-sbom.txt\n'
            'Use --force-reinstall --no-cache-dir options if it still fails\n'
            'Pass "--scancode=/path/to/scancode" if the scancode executable is'
            'not available on PATH.')
    try:
        command_execute(args.scancode, '--version', allow_stderr=True)
    except Exception as ex:
        raise SbomException(f'Cannot execute scancode command "{args.scancode}".\n'
            f'Make sure that you have scancode-toolkit installed.\n'
            f'Pass "--scancode=/path/to/scancode" if the scancode executable is '
            f'not available on PATH.') from ex


def get_scancode_workers() -> int:
    '''Returns number of parallel scancode invocations.'''
    if args.processes > 0:
        return args.processes
    cpu_count = os.cpu_count()
    if cpu_count is None or cpu_count < 1:
        return 1
    # dont know why but using maximum cores makes the system or sbom tool unstable
    return min(cpu_count, SCANCODE_DEFAULT_PARALLEL_WORKERS)


def run_scancode(file: FileInfo) -> 'dict|None':
    '''Execute scancode and get license identifier from its results.'''
    last_error = ''
    for attempt in range(SCANCODE_RUN_RETRIES + 1):
        with NamedTemporaryFile(mode='w+', delete=False) as output_file:
            output_path = output_file.name
        try:
            _, return_code = command_execute(args.scancode, '-cl',
                                             '--json', output_path,
                                             '--license-text',
                                             '--license-score', '100',
                                             '--license-text-diagnostics',
                                             '--quiet',
                                             '--processes', '1',
                                             file.file_path,
                                             allow_stderr=True,
                                             return_error_code=True)
            if return_code == 0:
                try:
                    with open(output_path, encoding='utf-8') as fd:
                        return json.load(fd)
                except Exception as ex:
                    last_error = f'Invalid JSON output: {ex}'
            else:
                last_error = f'Exit code {return_code}'
        except Exception as ex:
            last_error = str(ex)
        finally:
            with suppress(OSError):
                os.unlink(output_path)
        if attempt < SCANCODE_RUN_RETRIES:
            log.wrn(f'ScanCode failed for "{file.file_path}" ({last_error}); '
                    f'retrying ({attempt + 1}/{SCANCODE_RUN_RETRIES}).')
            sleep(0.2 * (attempt + 1))
    log.wrn(f'ScanCode failed for "{file.file_path}" and will be skipped ({last_error}).')
    return None


def apply_scancode_result(data: Data, file: FileInfo, result: dict):
    '''Parse one ScanCode result and update file/license structures.'''
    current = result['files'][0]
    licenses = current.get('license_detections')
    if licenses is None:
        licenses = current.get('licenses', [])

    for item in licenses:
        friendly_id = next((item.get(key) for key in (
            'spdx_license_key',
            'key',
            'license_expression_spdx',
            'license_expression',
        ) if item.get(key)), '')
        id = friendly_id.upper()
        if id in ('UNKNOWN-SPDX', 'LICENSEREF-SCANCODE-UNKNOWN-SPDX') or id == '':
            matched_text = item.get('matched_text')
            if matched_text is None:
                matched_text = next((
                    match.get('matched_text') for match in item.get('matches', [])
                    if isinstance(match, dict) and match.get('matched_text') is not None
                ), None)
            if matched_text:
                friendly_id = re.sub(
                    r'SPDX-License-Identifier:', '', matched_text, flags=re.I
                ).strip()
                friendly_id = friendly_id.rstrip('*/').strip()
                friendly_id = friendly_id.lstrip('/*').strip()
                id = friendly_id.upper()
        if id == '':
            log.wrn(f'Invalid response from scancode-toolkit, file: {file.file_path}')
            continue

        file.licenses.add(id)
        file.licenses_in_file.add(id)
        file.detectors.add('scancode-toolkit')

        if not is_spdx_license(id):
            name = item.get('name') or item.get('short_name')
            url = (
                item.get('spdx_url')
                or item.get('reference_url')
                or item.get('scancode_text_url')
            )
            if id in data.licenses:
                license = data.licenses[id]
                if license.is_expr:
                    continue
            else:
                license = License()
                data.licenses[id] = license
                license.id = id
                license.friendly_id = friendly_id
            if license.name is None:
                license.name = name
            if license.url is None:
                license.url = url
            license.detectors.add('scancode-toolkit')

    for item in current.get('copyrights', []):
        if not isinstance(item, dict):
            log.wrn(f'Invalid copyright response from scancode-toolkit, file: {file.file_path}')
            continue
        copyright_text = (item.get('copyright') or item.get('value') or '').strip()
        if copyright_text:
            file.copyright_texts.add(copyright_text)
            file.detectors.add('scancode-toolkit')


def detect(data: Data, optional: bool):
    '''License detection using scancode-toolkit.'''

    if optional:
        filtered = tuple(filter(lambda file: len(file.licenses_in_file) == 0, data.files))
    else:
        filtered = tuple(data.files)

    if len(filtered) > 0:
        check_scancode()

    workers = get_scancode_workers()
    skipped_files: list[FileInfo] = []
    if len(filtered) >= 2 and workers > 1:
        log.dbg(f'Starting {workers} parallel threads for scancode-toolkit detector')
        with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as executor:
            decoded = executor.map(run_scancode, filtered, chunksize=1)
            for result, file in zip(decoded, filtered, strict=False):
                if result is None:
                    skipped_files.append(file)
                    continue
                apply_scancode_result(data, file, result)
    else:
        for file in filtered:
            result = run_scancode(file)
            if result is None:
                skipped_files.append(file)
                continue
            apply_scancode_result(data, file, result)
    if len(skipped_files) > 0:
        log.wrn(f'ScanCode skipped {len(skipped_files)} file(s) due to errors.')
