#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import sys
import re
from pathlib import Path

from args import args
from common import SbomException, command_execute, concurrent_pool_iter
from data_structure import Data, FileInfo, Package
from west import log

                                                    # FileInfo is used.

_manifest_projects: 'dict[Path, object]' = {}
_manifest_path_index: 'list[tuple[Path, object]]' = []
_self_repo_path: 'Path|None' = None
_self_repo_name: 'str|None' = None
_self_repo_version: 'str|None' = None


def split_lines(text: str) -> 'tuple[str]':
    '''Split input text into stripped lines removing empty lines.'''
    return tuple(line.strip() for line in text.split('\n') if len(line.strip()))


def git_url_to_purl(git_url: str, version: str) -> 'str|None':
    '''Convert git URL to Package URL (PURL) format.

    Supports GitHub, GitLab, and Bitbucket URLs.
    Format: pkg:<type>/<namespace>/<name>@<version>
    '''
    if not git_url or not version:
        return None

    url = git_url.strip()
    if url.endswith('.git'):
        url = url[:-4]
    if url.startswith('git@'):
        url = url.replace(':', '/', 1).replace('git@', 'https://')

    git_services = [
        (r'https?://github\.com/([^/]+)/(.+)', 'github'),
        (r'https?://gitlab\.com/([^/]+)/(.+)', 'gitlab'),
        (r'https?://bitbucket\.org/([^/]+)/(.+)', 'bitbucket'),
    ]

    for pattern, purl_type in git_services:
        match = re.match(pattern, url)
        if match:
            namespace, name = match.groups()
            return f'pkg:{purl_type}/{namespace}/{name}@{version}'

    return f'pkg:generic/{url.split("/")[-1]}@{version}'


def extract_supplier_from_url(git_url: str) -> 'str|None':
    '''Extract supplier/organization name from git URL.'''
    if not git_url:
        return None

    url = git_url.strip()
    if url.endswith('.git'):
        url = url[:-4]
    if url.startswith('git@'):
        url = url.replace(':', '/', 1).replace('git@', 'https://')

    for pattern in [
        r'https?://github\.com/([^/]+)/',
        r'https?://gitlab\.com/([^/]+)/',
        r'https?://bitbucket\.org/([^/]+)/',
    ]:
        match = re.match(pattern, url)
        if match:
            return match.group(1)

    return None


def get_remote_url(path: Path, remote_name: str) -> str:
    '''Returns URL from specified git remote name and path.'''
    output = command_execute(args.git, 'remote', 'get-url', remote_name, cwd=path)
    return output.strip()


def get_origin(absolute_path: Path, relative_path: Path) -> 'str|None':
    '''Try to find out the git original source on specified path.'''
    output, error_code = command_execute(args.git, 'remote', cwd=absolute_path,
                                         return_error_code=True, allow_stderr=True)
    if error_code != 0:
        log.wrn(f'Directory "{relative_path}" does not provide valid git remote information.')
        return None
    remotes = split_lines(output)
    if len(remotes) == 1:
        return get_remote_url(absolute_path, remotes[0])
    git_urls = set()
    west_urls = set()
    for remote in remotes:
        git_urls.add(get_remote_url(absolute_path, remote))
    output, _ = command_execute(sys.argv[0], 'list', '-f', '{path}`{url}', cwd=absolute_path,
                                return_error_code=True, allow_stderr=True)
    projects = split_lines(output)
    for project in projects:
        pair = project.split('`')
        if len(pair) != 2:
            continue
        project_path, url = pair
        if ((not url.startswith('http')) and (not url.startswith('ssh')) and
            (not url.startswith('git'))):
            continue
        if ((str(relative_path) == project_path) or
            str(relative_path).startswith(project_path + '/') or
            str(relative_path).startswith(project_path + '\\')):
            west_urls.add(url)
    for url in git_urls.intersection(west_urls):
        return url
    for remote in remotes:
        if remote == 'origin':
            return get_remote_url(absolute_path, remote)
    for url in git_urls:
        return url
    return None


def get_sha(absolute_path: Path) -> 'str|None':
    ''' Returns git commit SHA on specified path. '''
    output, error_code = command_execute(args.git, 'rev-parse', 'HEAD',
                                         cwd=absolute_path, return_error_code=True,
                                         allow_stderr=True)
    output = output.strip()
    return output if (len(output) == 40) and (error_code == 0) else None


def get_toplevel(absolute_path: Path) -> 'Path|None':
    '''Returns the resolved git toplevel directory at `absolute_path`.'''
    output, error_code = command_execute(args.git, 'rev-parse', '--show-toplevel',
                                         cwd=absolute_path, return_error_code=True,
                                         allow_stderr=True, log_stderr=False)
    if error_code != 0:
        return None
    line = output.strip()
    if not line:
        return None
    try:
        return Path(line).resolve()
    except OSError:
        return None


def upstream_url(project) -> 'str|None':
    '''Returns userdata.ncs.upstream-url for a manifest project.'''
    userdata = getattr(project, 'userdata', None)
    if not isinstance(userdata, dict):
        return None
    ncs = userdata.get('ncs')
    if not isinstance(ncs, dict):
        return None
    url = ncs.get('upstream-url')
    return url.strip() if isinstance(url, str) and url.strip() else None


def read_version(version_file: Path, include_dev: bool = False) -> 'str|None':
    '''Returns the NCS version from `version_file` when PATCHLEVEL != 99.

    Accepts both historical single-line and common-format VERSION files.
    PATCHLEVEL == 99 returns None unless `include_dev` is set.
    '''
    try:
        text = version_file.read_text(encoding='utf-8').strip()
    except OSError:
        return None
    match = re.match(r'^(\d+)\.(\d+)\.(\d+)(?:-([A-Za-z0-9.\-]+))?$', text)
    if not match:
        match = re.fullmatch(
            r'VERSION_MAJOR\s*=\s*(\d+)\s+VERSION_MINOR\s*=\s*(\d+)\s+'
            r'PATCHLEVEL\s*=\s*(\d+)\s+VERSION_TWEAK\s*=\s*\d+\s+'
            r'EXTRAVERSION\s*=\s*([A-Za-z0-9.\-]*)'
            r'(?:\s+VERSION_METADATA\s*=\s*[A-Za-z0-9.\-]*)?', text)
    if not match:
        return None
    major, minor, patch = (int(x) for x in match.groups()[:3])
    extra = match.group(4)
    version = f'{major}.{minor}.{patch}' + (f'-{extra}' if extra else '')
    return None if patch == 99 and not include_dev else version


def load_manifest():
    '''Populate the module-level manifest caches consulted by detect_dir.'''
    global _manifest_projects, _manifest_path_index
    global _self_repo_path, _self_repo_name, _self_repo_version
    _manifest_projects = {}
    _manifest_path_index = []
    _self_repo_path = None
    _self_repo_name = None
    _self_repo_version = None
    try:
        from west.manifest import Manifest
        manifest = Manifest.from_topdir()
    except Exception:
        return
    for project in manifest.projects:
        abspath = getattr(project, 'abspath', None)
        if not abspath:
            continue
        try:
            key = Path(abspath).resolve()
        except OSError:
            continue
        _manifest_path_index.append((key, project))
        if getattr(project, 'url', None):
            _manifest_projects[key] = project
        elif _self_repo_path is None:
            # by west convention projects[0] is the manifest repo.
            _self_repo_path = key
            _self_repo_name = getattr(project, 'name', None) or key.name
    if _self_repo_path is not None:
        _self_repo_version = read_version(_self_repo_path / 'VERSION')

    _manifest_path_index.sort(key=lambda item: len(item[0].parts), reverse=True)


def match_manifest_project(absolute_path: Path) -> 'tuple[Path, object]|None':
    '''Return (project_root, project) for the manifest project whose directory is the
    longest prefix of absolute_path, or None.'''
    try:
        target = absolute_path.resolve()
    except OSError:
        target = absolute_path
    parents = set(target.parents)
    for key, project in _manifest_path_index:
        if target == key or key in parents:
            return (key, project)
    return None


def detect_dir(func_args: 'tuple[list[FileInfo],Data]') -> None:
    '''Read input file content and try to detect licenses by its content.'''
    files = func_args[0]
    data = func_args[1]
    files_to_assign = [file for file in files if file.package in ('', None)]
    if len(files_to_assign) == 0:
        return
    modified_files = set()
    untracked_files = set()
    absolute_path = Path(files_to_assign[0].file_path).parent
    relative_path = Path(files_to_assign[0].file_rel_path).parent
    repo = get_toplevel(absolute_path)
    if repo is None:
        log.wrn(f'Directory "{relative_path}" is not a git repository. '
                'Files will be included without git-info detector information.')
        git_sha = None
        git_origin = None
    else:
        git_sha = get_sha(absolute_path)
        git_origin = get_origin(absolute_path, relative_path)
    project = _manifest_projects.get(repo) if repo is not None else None
    if project is not None:
        git_origin = project.url
        git_sha = project.revision or git_sha
    elif _self_repo_version is not None and repo == _self_repo_path:
        git_sha = _self_repo_version
    # Fall back to the west manifest when git provided no origin.
    # This resolves package identity from the manifest without any git remote.
    package_name = None
    manifest_root = None
    if git_origin is None:
        match = match_manifest_project(absolute_path)
        if match is not None:
            project_root, mproject = match
            manifest_root = project_root
            git_origin = getattr(mproject, 'url', None) or None
            git_sha = git_sha or getattr(mproject, 'revision', None)
            if git_origin is None:
                # The url-less self/manifest repo: resolve it by name only.
                package_name = getattr(mproject, 'name', None) or project_root.name
                if project_root == _self_repo_path and _self_repo_version is not None:
                    git_sha = _self_repo_version
            if project is None:
                project = mproject

    if repo is not None:
        output, error_code = command_execute(args.git, 'status', '--porcelain', '--ignored',
                                             '--untracked-files=all', '*', cwd=absolute_path,
                                             return_error_code=True, allow_stderr=True)
        if error_code != 0:
            log.wrn(f'Directory "{absolute_path}" does not provide valid git status information.')
        for line in split_lines(output):
            change_type = line[0:2]
            if change_type == '  ':
                continue
            if change_type == '!!':
                untracked_files.add(Path(line[3:]).name.upper())
                continue
            modified_files.add(Path(line[3:]).name.upper())
    if (git_origin is not None) and (git_sha is not None):
        package_id = f'git#{git_origin}#{git_sha}'.upper()
    elif package_name is not None:
        version_tag = git_sha if git_sha is not None else 'NONE'
        package_id = f'pkg#{package_name}#{version_tag}'.upper()
    else:
        package_id = ''
    if package_id not in data.packages:
        package = Package()
        package.id = package_id
        package.url = git_origin
        package.version = git_sha
        package.root_path = repo or manifest_root
        if git_origin is None and package_name is not None:
            package.name = package_name
        if git_origin and git_sha:
            package.purl = git_url_to_purl(git_origin, git_sha)
            package.supplier = args.package_supplier or extract_supplier_from_url(git_origin)
        if project is not None:
            package.browser_url = upstream_url(project)
        if args.package_cpe:
            package.cpe = args.package_cpe
        data.packages[package_id] = package
    for file in files_to_assign:
        file.package = package_id
        file_name = Path(file.file_rel_path).name.upper()
        if file_name in modified_files:
            file.local_modifications = True
        elif file_name in untracked_files:
            file.package = ''


def check_external_tools():
    '''
    Checks if "git" command works correctly. If not, raises exception with information
    for user.
    '''
    try:
        command_execute(args.git, '--version', allow_stderr=True)
    except BaseException as ex:
        # We are checking if calling this command works at all,
        # so ANY kind of problem (exception) should return "False" value.
        raise SbomException('Cannot execute "git" command.\nMake sure it available on your '
                            'PATH or provide it with "--git" argument.') from ex


def detect(data: Data, optional: bool):
    ''' Fill "data" with version information obtained with the "git". '''

    check_external_tools()
    load_manifest()

    group_by_dir = {}
    for file in data.files:
        if '.git' in file.file_rel_path.parts:
            continue
        dir_name = str(file.file_rel_path.parent)
        if dir_name not in group_by_dir:
            group_by_dir[dir_name] = ([], data)
        group_by_dir[dir_name][0].append(file)

    for _, _, _ in concurrent_pool_iter(detect_dir, group_by_dir.values()):
        pass
