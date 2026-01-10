#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Implementation of a detector that searches for a license information in an external file.
The external file must:
 * be located in the same directory or any of the parent directories.
 * have a name containing "LICENSE", "LICENCE" or "COPYING".
 * contain one SPDX tag and one or more "NCS-SBOM-Apply-To-File" tags.
The "NCS-SBOM-Apply-To-File" tag value is a file path or a glob (see pathlib.Path.glob)
relative directory where the external file is located. The value ends at the end of line,
so don't use any comment closing characters or whitespaces at the end of the line.
'''

import fnmatch
import os
import re
import traceback
from pathlib import Path

from data_structure import Data
from west import log

SPDX_TAG_RE = re.compile(
    r'(?:^|[^a-zA-Z0-9\-])SPDX-License-Identifier\s*:\s*([a-zA-Z0-9 :\(\)\.\+\-]+)',
    re.IGNORECASE)

APPLY_TO_FILES_TAG_RE = re.compile(
    r'(?:^|[^a-zA-Z0-9\-])NCS-SBOM-Apply-To-File\s*:\s*(?:"(.*?[^\\])"|([^\s]+))',
    re.IGNORECASE)

EXTERNAL_FILE_RE = re.compile(
    r'LICENSE|LICENCE|COPYING',
    re.IGNORECASE)


detected_files = dict()
dir_search_done = set()


def parse_license_file(file: Path):
    try:
        with open(file, encoding='8859') as fd:
            content = fd.read()
    except: # pylint: disable=bare-except
        # Going up to root directory may cause some unexpected IO/permission problems.
        # It is ok to ignore them all, because we can assume that a valid external file
        # must be accessible.
        log.dbg(f'Exception reading file "{file}": {traceback.format_exc()}',
                level=log.VERBOSE_VERY)
        return None
    licenses = set()
    for m in SPDX_TAG_RE.finditer(content):
        id = m.group(1).strip()
        if id != '':
            licenses.add(id.upper())
    if len(licenses) == 0:
        return None
    globs = list()
    for m in APPLY_TO_FILES_TAG_RE.finditer(content):
        if m.group(1) is not None:
            glob = re.sub(r'\\(.)', r'\1', m.group(1).strip())
        else:
            glob = m.group(2).strip()
        globs.append(glob)
    return (licenses, globs)


def load_external_globs() -> 'list[tuple[str,set[str]]]':
    external_globs = list()
    external_globs_dir = Path(__file__).parent / 'data' / 'external-licenses'
    if not external_globs_dir.exists():
        return external_globs
    try:
        listdir = list(external_globs_dir.iterdir())
    except: # pylint: disable=bare-except
        log.dbg(f'Exception reading directory "{external_globs_dir}": {traceback.format_exc()}',
                level=log.VERBOSE_VERY)
        return external_globs
    for file in listdir:
        if (file.is_file() and EXTERNAL_FILE_RE.search(file.name) is not None):
            parsed = parse_license_file(file)
            if parsed is None:
                continue
            licenses, globs = parsed
            log.dbg(f'External file {file} with {licenses}')
            for glob in globs:
                pattern = glob.replace('\\', '/')
                if os.name == 'nt':
                    pattern = pattern.lower()
                external_globs.append((pattern, licenses))
    return external_globs


def search_dir(directory: Path):
    if str(directory) in dir_search_done:
        return
    try:
        listdir = list(directory.iterdir())
    except: # pylint: disable=bare-except
        # Going up to root directory may cause some unexpected IO/permission problems.
        # It is ok to ignore them all, because we can assume that a valid external file
        # must be accessible.
        log.dbg(f'Exception reading directory "{directory}": {traceback.format_exc()}',
                level=log.VERBOSE_VERY)
        listdir = list()
    for file in listdir:
        if (file.is_file() and EXTERNAL_FILE_RE.search(file.name) is not None):
            parsed = parse_license_file(file)
            if parsed is None:
                continue
            licenses, globs = parsed
            log.dbg(f'External file {file} with {licenses}')
            for glob in globs:
                log.dbg(f'  describes {glob}:', level=log.VERBOSE_EXTREME)
                try:
                    matched_files = tuple(file.parent.glob(glob))
                except ValueError as ex:
                    log.wrn(f'Invalid glob "{glob}": {ex}')
                    continue
                for matched in matched_files:
                    matched_str = str(matched)
                    log.dbg(f'    {matched_str}', level=log.VERBOSE_EXTREME)
                    if matched_str not in detected_files:
                        detected_files[matched_str] = set()
                    detected_files[matched_str].update(licenses)
    dir_search_done.add(str(directory))
    if directory.parent != directory:
        search_dir(directory.parent)


def detect(data: Data, optional: bool):
    '''External file detector.'''

    external_globs = load_external_globs()

    if optional:
        filtered = filter(lambda file: len(file.licenses) == 0, data.files)
    else:
        filtered = data.files

    for file in filtered:
        search_dir(file.file_path.parent)
        if str(file.file_path) in detected_files:
            file.licenses.update(detected_files[str(file.file_path)])
            file.detectors.add('external-file')
        if external_globs:
            path_str = file.file_path.as_posix()
            if os.name == 'nt':
                path_str = path_str.lower()
            for pattern, licenses in external_globs:
                if fnmatch.fnmatchcase(path_str, pattern):
                    file.licenses.update(licenses)
                    file.detectors.add('external-file')
