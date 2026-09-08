#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Generates report using the Jinja2 templates.
'''

import hashlib
import os
from datetime import datetime, timezone
from pathlib import Path
from urllib.parse import quote

from args import args
from common import is_sha
from data_structure import Data, FileInfo, Package  # pylint: disable=unused-import
from git_info_detector import read_version
from jinja2 import Template, filters
from west import log

                                           # Ignoring false warning from pylint, FileInfo is used.


counter_value = 0


def verification_code(files: 'list[FileInfo]') -> str:
    '''Calculate verification code'''
    files.sort(key=lambda f: f.sha1)
    sha1 = hashlib.sha1()
    for file in files:
        sha1.update(file.sha1.encode('utf-8'))
    return sha1.hexdigest()


def adjust_identifier(license: str) -> str:
    '''Adjust LicenseRef identifier'''
    return license.replace('LICENSEREF', 'LicenseRef')


def sanitize_tagvalue_text(value: str) -> str:
    '''Prevent copyright text from terminating an SPDX tag-value text block.'''
    return str(value).replace('</text>', '&lt;/text&gt;')


filters.FILTERS['verification_code'] = verification_code
filters.FILTERS['adjust_identifier'] = adjust_identifier
filters.FILTERS['sanitize_tagvalue_text'] = sanitize_tagvalue_text


def group_by(files: 'list[FileInfo]', attr_name: str) -> 'dict[list[FileInfo]]':
    result = dict()
    for file in files:
        attr_value = getattr(file, attr_name)
        if attr_value not in result:
            result[attr_value] = list()
        result[attr_value].append(file)
    return result


def counter() -> int:
    global counter_value
    counter_value += 1
    return counter_value


def package_roots(data: Data) -> 'dict[str,Path]':
    '''Resolved root directory of each package that provides one.'''
    roots = {}
    for package_id, package in data.packages.items():
        if package.root_path is None:
            continue
        try:
            roots[package_id] = Path(package.root_path).resolve()
        except OSError:
            continue
    return roots


def strip_anchor(path: Path) -> str:
    '''POSIX form of an absolute path.'''
    return path.relative_to(path.anchor).as_posix() if path.anchor else path.as_posix()


def spdx_file_name(file: FileInfo, roots: 'dict[str,Path]') -> str:
    '''SPDX FileName of a file: a POSIX path relative to the root of its package.
    See SPDX 2.3 clause 8.1: the name is relative to the package root and starts with "./".
    '''
    root = roots.get(file.package)
    if root is not None:
        try:
            return './' + Path(file.file_path).resolve().relative_to(root).as_posix()
        except (ValueError, OSError):
            pass
    workspace_path = Path(file.file_rel_path)
    if not workspace_path.is_absolute() and '..' not in workspace_path.parts:
        return './' + workspace_path.as_posix()
    return './' + strip_anchor(Path(file.file_path).resolve())


def timestamp() -> str:
    return datetime.now(timezone.utc).isoformat(timespec='seconds').replace('+00:00', 'Z')


def github_archive_url(git_url: str, version: str) -> 'str|None':
    '''Convert a GitHub URL and revision to a downloadable archive zip URL.'''
    url = git_url.strip()
    if url.endswith('.git'):
        url = url[:-4]
    if 'github.com' not in url:
        return None
    parts = url.split('github.com', 1)[1].lstrip(':/').split('/')
    if len(parts) < 2 or not parts[0] or not parts[1]:
        return None
    org, repo = parts[0], parts[1]
    if is_sha(version):
        return f'https://github.com/{org}/{repo}/archive/{version}.zip'
    return f'https://github.com/{org}/{repo}/archive/refs/tags/{version}.zip'


def download_location(package: Package) -> str:
    '''Format the SPDX PackageDownloadLocation field for a package.'''
    if package.purl:
        if args.package_download_format == 'github-archive':
            archive_url = github_archive_url(package.url, package.version)
            if archive_url is not None:
                return archive_url
        return f'git+{package.url}@{package.version}'
    return package.url or 'NONE'


def get_ncs_version() -> 'str|None':
    '''NCS version from the repo's root VERSION file, or None if missing.'''
    try:
        version_file = Path(__file__).resolve().parents[3] / 'VERSION'
        if version_file.is_file():
            return read_version(version_file, include_dev=True)
    except Exception:
        pass
    return None


def data_to_dict(data: Data) -> dict:
    '''Convert object to dict by copying public attributes to a new dictionary.'''
    result = dict()
    for name in dir(data):
        if name.startswith('_'):
            continue
        result[name] = getattr(data, name)
    roots = package_roots(data)
    result['func'] = {
        'group_by': group_by,
        'counter': counter,
        'spdx_file_name': lambda file: spdx_file_name(file, roots),
        'timestamp': timestamp,
        'download_location': download_location,
    }
    ncs_version = get_ncs_version()
    result['ncs_version_suffix'] = f'-{ncs_version}' if ncs_version else ''
    return result


def generate(data: Data, output_file: 'Path|str', template_file: Path):
    '''Generate output_file from data using template_file.'''
    output_file = Path(output_file)
    log.dbg(f'Writing output to "{output_file}" using template "{template_file}"')
    with open(template_file, encoding='utf-8') as fd:
        template_source = fd.read()
    t = Template(template_source)
    out = t.render(**data_to_dict(data))
    with open(output_file, 'w', encoding='utf-8') as fd:
        fd.write(out)
    escaped_path = quote(str(output_file.resolve()).replace(os.sep, '/').strip("/"))
    log.inf(f'Output written to file:///{escaped_path}')
