#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Implementation of a detector that extracts licenses and copyright notices using the REUSE tool.
For more details see: https://reuse.software/

REUSE reads the file, its ".license" file and REUSE.toml.
'''

import re
from functools import cache
from pathlib import Path

from common import concurrent_pool_iter
from data_structure import Data, FileInfo
from west import log

# The characters of a license expression, as in the spdx-tag detector. REUSE may add text after it.
LICENSE_RE = re.compile(r'\(*[a-zA-Z0-9][a-zA-Z0-9 :().+-]*')
# The sources of the REUSE information that are a part of the file.
SOURCES_IN_FILE = ('file-header', 'dot-license')


@cache
def reuse_toml_dirs(directory: Path) -> 'tuple[Path, ...]':
    '''Get the directories containing REUSE.toml, from the provided directory and its parents.

    The topmost directory is the first one.
    '''
    parents = () if directory.parent == directory else reuse_toml_dirs(directory.parent)
    return (*parents, directory) if (directory / 'REUSE.toml').is_file() else parents


@cache
def get_project(dirs: 'tuple[Path, ...]'):
    '''Get the REUSE project with the topmost directory as the root and all REUSE.toml files.

    A REUSE.toml file applies to the files below it, so the files of the child directories are
    taken into account, with the same precedence rules as in REUSE. Project.from_directory() is not
    used, because it searches the entire directory tree for nested REUSE.toml files, which takes
    seconds on a large repository.
    '''
    from reuse.exceptions import ReuseError
    from reuse.global_licensing import NestedReuseTOML, ReuseTOML
    from reuse.project import Project

    tomls = []
    for toml_path in (d / 'REUSE.toml' for d in dirs):
        if toml_path.is_file():
            try:
                tomls.append(ReuseTOML.from_file(toml_path))
            except (ReuseError, OSError) as ex:
                log.wrn(f'Ignoring invalid REUSE configuration "{toml_path}": {ex}')
    global_licensing = NestedReuseTOML(reuse_tomls=tomls, source=str(dirs[0])) if tomls else None
    return Project(dirs[0], global_licensing=global_licensing)


def detect_file(file: FileInfo) -> 'tuple[set[str], set[str], set[str]]':
    '''Detect copyright notices, licenses, and licenses in the file by the file content.'''
    from reuse.exceptions import ReuseError

    path = Path(file.file_path).resolve()
    project = get_project(reuse_toml_dirs(path.parent) or (path.parent,))
    # ReuseError means that REUSE cannot parse the content, for example a copyright notice in a C
    # string that ends with a line break escape.
    try:
        infos = project.reuse_info_of(path)
    except (OSError, ValueError, LookupError, ReuseError) as ex:
        log.wrn(f'Cannot detect licenses and copyright notices in "{path}": {ex}')
        return set(), set(), set()

    notices = {
        str(n).strip()
        for i in infos
        for n in i.copyright_notices
        if (n.name or n.years) and n.name not in ('NONE', 'NOASSERTION')
    }

    notices = {n for n in notices if n.isprintable() and '\N{REPLACEMENT CHARACTER}' not in n}
    licenses = set()
    licenses_in_file = set()
    for info in infos:
        found = {
            m.group().strip().upper()
            for e in info.spdx_expressions
            if (m := LICENSE_RE.match(str(e).strip()))
        }
        licenses |= found
        if getattr(info.source_type, 'value', None) in SOURCES_IN_FILE:
            licenses_in_file |= found
    return notices, licenses, licenses_in_file


def detect(data: Data, optional: bool):
    '''License and copyright detection using the REUSE tool.

    If optional, the licenses are added only to the files that have no license yet, and the
    copyright notices only to the files that have no copyright notice yet.
    '''
    files = tuple(data.files)
    if optional:
        files = tuple(f for f in files if not f.licenses or not f.copyright_texts)

    for (notices, licenses, licenses_in_file), file, _ in concurrent_pool_iter(
        detect_file, files, True, 4096
    ):
        if licenses and (not optional or not file.licenses):
            file.licenses.update(licenses)
            file.licenses_in_file.update(licenses_in_file)
            file.detectors.add('reuse')
        if notices and (not optional or not file.copyright_texts):
            file.copyright_texts.update(notices)
            file.detectors.add('reuse')
