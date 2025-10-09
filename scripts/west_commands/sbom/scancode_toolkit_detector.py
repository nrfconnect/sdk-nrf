#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Implementation of a detector based on an external tool - scancode-toolkit.
For more details see: https://scancode-toolkit.readthedocs.io/en/stable/
'''

import json
import os
import re
from tempfile import NamedTemporaryFile
from west import log
from data_structure import Data, FileInfo, License
from args import args
from common import SbomException, command_execute
from license_utils import is_spdx_license


def check_scancode():
    '''Checks if "scancode --version" works correctly. If not, raises exception with information
    for user.'''
    try:
        command_execute(args.scancode, '--version', allow_stderr=True)
    except Exception as ex:
        raise SbomException(f'Cannot execute scancode command "{args.scancode}".\n'
            f'Make sure that you have scancode-toolkit installed.\n'
            f'Pass "--scancode=/path/to/scancode" if the scancode executable is '
            f'not available on PATH.') from ex


def run_scancode(file: FileInfo) -> 'set(str)':
    '''Execute scancode and get license identifier from its results.'''
    with NamedTemporaryFile(mode="w+", delete=False) as output_file:
        command_execute(args.scancode, '-cl',
                        '--json', output_file.name,
                        '--license-text',
                        '--license-score', '100',
                        '--license-text-diagnostics',
                        '--quiet',
                        f'-n {args.processes if args.processes > 0 else os.cpu_count()}', # Does not observe grow of efficiency on single file run.
                        file.file_path,
                        allow_stderr=True)
        output_file.seek(0)
        result = json.loads(output_file.read())
        output_file.close()
        os.unlink(output_file.name)
        return result


def detect(data: Data, optional: bool):
    '''License detection using scancode-toolkit.'''

    if optional:
        filtered = tuple(filter(lambda file: len(file.licenses) == 0, data.files))
    else:
        filtered = data.files

    if len(filtered) > 0:
        check_scancode()

    decoded = map(run_scancode, filtered)

    for result, file in zip(decoded, filtered):

        current = result['files'][0]
        if 'licenses' in current:
            licenses = current['licenses']
        elif 'license_detections' in current:
            licenses = current['license_detections']
        else:
            print('No license information for {}'.format(current['path']))
            continue

        for i in licenses:

            friendly_id = ''
            if 'spdx_license_key' in i and i['spdx_license_key'] != '':
                friendly_id = i['spdx_license_key']
            elif 'key' in i and i['key'] != '':
                friendly_id = i['key']
            elif 'license_expression_spdx' in i and i['license_expression_spdx'] != '':
                friendly_id = i['license_expression_spdx']
            elif 'license_expression' in i and i['license_expression'] != '':
                friendly_id = i['license_expression']
            id = friendly_id.upper()
            if id in ('UNKNOWN-SPDX', 'LICENSEREF-SCANCODE-UNKNOWN-SPDX') or id == '':
                matched_text = None
                if 'matched_text' in i:
                    matched_text = i['matched_text']
                elif 'matches' in i and isinstance(i['matches'], list):
                    matched_text = next((match.get('matched_text')
                                         for match in i['matches']
                                         if match.get('matched_text') is not None), None)
                if matched_text:
                    friendly_id = re.sub(r'SPDX-License-Identifier:', '', matched_text,
                                         flags=re.I).strip()
                    friendly_id = friendly_id.rstrip('*/').strip()
                    friendly_id = friendly_id.lstrip('/*').strip()
                    id = friendly_id.upper()
            if id == '':
                log.wrn(f'Invalid response from scancode-toolkit, file: {file.file_path}')
                continue

            file.licenses.add(id)
            file.detectors.add('scancode-toolkit')

            if not is_spdx_license(id):
                if 'name' in i:
                    name = i['name']
                elif 'short_name' in i:
                    name = i['short_name']
                else:
                    name = None

                if 'spdx_url' in i:
                    url = i['spdx_url']
                elif 'reference_url' in i:
                    url = i['reference_url']
                elif 'scancode_text_url' in i:
                    url = i['scancode_text_url']
                else:
                    url = None

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
