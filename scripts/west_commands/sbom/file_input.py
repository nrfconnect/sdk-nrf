#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Input files from command line and/or a text file.
'''

import re
import os
from pathlib import Path
from typing import Generator
from west import log, util
from args import args
from common import SbomException
from data_structure import Data, FileInfo


GLOB_PATTERN_START = re.compile(r'[\*\?\[]')


def is_glob(glob: str) -> bool:
    '''Returns True if provided string is a glob.'''
    return GLOB_PATTERN_START.search(glob) is not None


def glob_with_abs_patterns(path: Path, glob: str) -> Generator:
    '''Wrapper for Path.glob allowing absolute patterns in the glob input.'''
    glob_path = Path(glob)
    try:
        if glob_path.is_absolute():
            m = GLOB_PATTERN_START.search(glob)
            if m is None:
                return (glob_path, )
            parent = Path(glob[:m.start() + 1]).parent
            relative = glob_path.relative_to(parent)
            return tuple(parent.glob(str(relative)))
        else:
            return tuple(path.glob(glob))
    except ValueError as ex:
        raise SbomException(f'Invalid glob "{glob}": {ex}') from ex


def resolve_globs(path: Path, globs: 'list[str]') -> 'set(Path)':
    '''Resolves list of globs (optionally with "!" at the beginning) are returns a set of files.'''
    result = set()
    for glob in globs:
        if glob.startswith('!'):
            for file in glob_with_abs_patterns(path, glob[1:]):
                result.discard(file)
        else:
            added_files = 0
            for file in glob_with_abs_patterns(path, glob):
                if file.is_file():
                    result.add(file)
                    added_files += 1
            if added_files == 0:
                if is_glob(glob):
                    log.wrn(f'Input glob "{glob}" does not match any file.')
                else:
                    log.err(f'Input file "{glob}" does not exists.')
                    raise SbomException('Invalid input')
    return result


def generate_input(data: Data):
    '''Generate input files from list of files/globs from command line and/or a text file.'''
    full_set = set()

    if args.input_files is not None:
        cwd = Path('.').resolve()
        for globs in args.input_files:
            joiner = '", "'
            data.inputs.append(f'Files: "{joiner.join(globs)}" (relative to "{cwd}")')
            r = resolve_globs(cwd, globs)
            full_set.update(r)

    if args.input_list_file is not None:
        for file in args.input_list_file:
            file_path = Path(file).resolve()
            data.inputs.append(f'A list of files read from: "{file_path}"')
            globs = list()
            try:
                with open(file_path, 'r') as fd:
                    for line in fd:
                        line = line.strip()
                        if line == '' or line[0] == '#':
                            continue
                        globs.append(line)
            except Exception as e:
                raise SbomException(f'Cannot read from file "{file_path}": {e}') from e
            r = resolve_globs(file_path.parent, globs)
            full_set.update(r)

    for file_path in full_set:
        file = FileInfo()
        file.file_path = file_path
        file.file_rel_path = Path(os.path.relpath(file_path, util.west_topdir()))
        data.files.append(file)
