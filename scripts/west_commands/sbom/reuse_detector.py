#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Implementation of a detector that extracts copyright notices using the REUSE tool.
For more details see: https://reuse.software/

It is fast enough to run on every input file, unlike the scancode-toolkit detector. REUSE finds
the candidate notices in the file, in its ".license" file or in REUSE.toml. This module decides
which of them are real notices and keeps their wording from the file, because REUSE's own string
form rewrites them, for example "(c) since 2020 Foo" becomes "(C) 2020 since Foo".
'''

import re
from functools import cache
from pathlib import Path

from common import SbomException, concurrent_pool_iter
from data_structure import Data, FileInfo
from west import log

# Control characters (except tab) and U+FFFD: a binary file that was decoded as text.
NOT_TEXT_CHARS = frozenset(map(chr, [*range(0x20), 0x7F, 0xFFFD])) - {'\t'}
SPDX_TAG = r'SPDX-(?:File|Snippet)CopyrightText:'
# SPDX values that mean "no copyright" and "not known". They do not name a copyright holder.
SPDX_NO_HOLDER = ('NONE', 'NOASSERTION')
NOTICE_START_RE = re.compile(SPDX_TAG + r'|Copyright(?=\s|\(|©)|©')
SPDX_TAG_RE = re.compile(SPDX_TAG + r'\s*')
COMMENT_END_RE = re.compile(r'''\s*(?:\*+/|--!?>|"""|\'\'\'|"\s*/*>|'\s*/*>|\*+)\s*$''')
# The longest prefix that does not name a holder. Always matches, possibly the empty string.
MARKER_RE = re.compile(r'(?:Copyright)?\s*(?P<symbol>©|\([Cc]\))?\s*')
YEAR_RE = re.compile(r'(?<!\d)(?:19|20)\d{2}(?!\d)')


def has_alnum(text: str) -> bool:
    '''Check if the text contains any letter or digit.'''
    return any(char.isalnum() for char in text)


def notice_text(line: str) -> 'str|None':
    '''Get the copyright notice from a line reported by REUSE.

    REUSE matches any line that contains "Copyright" followed by a space, or the copyright symbol.
    It reports the whole line, including the comment markers.

    @param line  Line containing a candidate notice.
    @returns     The notice in the wording of the file or None if this is not a notice.
    '''
    if not NOT_TEXT_CHARS.isdisjoint(line):
        return None
    start = NOTICE_START_RE.search(line)
    if start is None:
        return None
    text = COMMENT_END_RE.sub('', line[start.start() :]).strip()

    tagged = SPDX_TAG_RE.match(text)
    if tagged:
        text = text[tagged.end() :]
        if text in SPDX_NO_HOLDER:
            return None
        if not text.startswith(('Copyright', '©', '(c)', '(C)')):
            text = f'Copyright {text}'

    marker = MARKER_RE.match(text)
    holder = text[marker.end() :]
    if not has_alnum(holder):
        return None
    if tagged or YEAR_RE.search(text):
        return text

    # With no year and no SPDX tag this can be prose that mentions copyright, so be stricter.
    if has_alnum(line[: start.start()]):
        return None
    if text.startswith('©'):
        # A binary file decoded as text can contain "©" followed by random letters, for example
        # "©ÎwÈÍ", so expect a space and a capital letter, as in "© Acme Corporation".
        if not text[1:2].isspace() or not holder[0].isupper():
            return None
    elif marker['symbol'] is None and not holder[0].isupper():
        return None
    return text


@cache
def reuse_toml_dir(directory: Path) -> 'Path|None':
    '''Get the closest directory containing REUSE.toml, starting from the provided directory.'''
    if (directory / 'REUSE.toml').is_file():
        return directory
    return None if directory.parent == directory else reuse_toml_dir(directory.parent)


@cache
def get_project(root: Path):
    '''Get the REUSE project with the provided root directory.

    It is not created with Project.from_directory(), because it searches the entire directory tree
    for nested REUSE.toml files, which takes seconds on a large repository.
    '''
    from reuse.exceptions import GlobalLicensingParseError
    from reuse.global_licensing import ReuseTOML
    from reuse.project import Project

    global_licensing = None
    toml_path = root / 'REUSE.toml'
    if toml_path.is_file():
        try:
            global_licensing = ReuseTOML.from_file(toml_path)
        except (GlobalLicensingParseError, OSError) as ex:
            log.wrn(f'Ignoring invalid REUSE configuration "{toml_path}": {ex}')
    return Project(root, global_licensing=global_licensing)


def detect_file(file: FileInfo) -> 'set[str]':
    '''Read input file content and detect copyright notices by its content.'''
    path = Path(file.file_path).resolve()
    project = get_project(reuse_toml_dir(path.parent) or path.parent)
    # Other exceptions are not caught on purpose. They mean that the REUSE API has changed, which
    # must not silently look like files without a copyright.
    try:
        infos = project.reuse_info_of(path)
    except (OSError, ValueError, LookupError) as ex:
        log.wrn(f'Cannot detect copyright notices in "{path}": {ex}')
        return set()
    lines = (notice.to_string(original=True) for info in infos for notice in info.copyright_notices)
    return {text for text in map(notice_text, lines) if text is not None}


def check_reuse():
    '''Checks if the "reuse" package works correctly. If not, raises exception with information
    for user.'''
    try:
        # CopyrightNotice is the part of API that this detector needs. It was added in REUSE 6.0.
        from reuse.copyright import CopyrightNotice  # noqa: F401
        from reuse.project import Project  # noqa: F401
    except Exception as ex:
        raise SbomException(
            'Cannot use the "reuse" Python package, version 6.0.0 or newer is '
            f'required: {ex}\n'
            'Install the SBOM requirements with:\n'
            '  pip3 install -r scripts/requirements-west-ncs-sbom.txt\n'
            'Remove "reuse" from the "--license-detectors" option to generate the report '
            'without copyright information.'
        ) from ex


def detect(data: Data, optional: bool):
    '''Copyright detection using the REUSE tool.'''

    if optional:
        filtered = tuple(filter(lambda file: len(file.copyright_texts) == 0, data.files))
    else:
        filtered = tuple(data.files)

    if len(filtered) > 0:
        check_reuse()

    for results, file, _ in concurrent_pool_iter(detect_file, filtered, True, 4096):
        if len(results) > 0:
            file.copyright_texts.update(results)
            file.detectors.add('reuse')
