#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Implementation of a detector based on spdx identifiers.
For more details see: https://spdx.github.io/spdx-spec/using-SPDX-short-identifiers-in-source-files
'''

import re
from west import log
from data_structure import Data, FileInfo
from common import SbomException, concurrent_pool_iter


SPDX_TAG_RE = re.compile(
    r'(?:^|[^a-zA-Z0-9\-])SPDX-License-Identifier\s*:\s*([a-zA-Z0-9 :\(\)\.\+\-]+)',
    re.IGNORECASE)


def detect_file(file: FileInfo) -> 'set(str)':
    '''Read input file content and try to detect licenses by its content.'''
    try:
        with open(file.file_path, 'r', encoding='8859') as fd:
            content = fd.read()
    except Exception as e:
        log.err(f'Error reading file "{file.file_path}": {e}')
        raise SbomException() from e
    results = set()
    for m in SPDX_TAG_RE.finditer(content):
        id = m.group(1).strip()
        if id != '':
            results.add(id.upper())
    return results


def detect(data: Data, optional: bool):
    '''SPDX-tag detector.'''

    if optional:
        filtered = filter(lambda file: len(file.licenses) == 0, data.files)
    else:
        filtered = data.files

    for results, file, _ in concurrent_pool_iter(detect_file, filtered, True, 4096):
        if len(results) > 0:
            file.licenses.update(results)
            file.detectors.add('spdx-tag')
