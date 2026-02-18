#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Get input files from an application build directory.
'''

from __future__ import annotations

import os
import pickle
import platform
import re
import shlex
import shutil
from pathlib import Path
from typing import TYPE_CHECKING

import yaml
from west import log, util

from args import args
from common import SbomException, command_execute
from data_structure import Data, FileInfo, DataBaseClass, Package

if TYPE_CHECKING:
    from ninja_build_extractor import BuildArchive, BuildObject


DEFAULT_BUILD_DIR = 'build'
DEFAULT_TARGET = 'zephyr/zephyr.elf'
DEFAULT_GN_TARGET = 'default'
SOURCE_CODE_SUFFIXES = ['.c', '.cpp', '.cxx', '.cc', '.c++', '.s']


def get_sysbuild_default_domain(build_dir: Path) -> Path | None:
    '''
    Returns build directory of the default sysbuild domain if domains.yaml is present.
    '''
    domains_yaml = build_dir / 'domains.yaml'
    if not domains_yaml.is_file():
        return None
    try:
        with open(domains_yaml, 'r') as fd:
            domains = yaml.safe_load(fd)
    except yaml.YAMLError as ex:
        raise SbomException(f'Cannot parse "{domains_yaml}".') from ex
    if not isinstance(domains, dict) or 'default' not in domains or 'domains' not in domains:
        raise SbomException(f'Invalid format of "{domains_yaml}".')
    default_domain = domains.get('default')
    for domain in domains.get('domains', []):
        if not isinstance(domain, dict):
            continue
        if domain.get('name') == default_domain:
            domain_dir = Path(domain.get('build_dir', ''))
            if not domain_dir.exists():
                raise SbomException(f'Detected sysbuild domain "{default_domain}", but the '
                                    f'build directory "{domain_dir}" does not exist.')
            log.dbg(f'Using sysbuild domain "{default_domain}" build directory "{domain_dir}".')
            return domain_dir
    raise SbomException(f'Cannot find sysbuild default domain "{default_domain}" in '
                        f'"{domains_yaml}".')


def get_sysbuild_domains(build_dir: Path) -> list[tuple[str, Path]]:
    '''
    Returns list of (domain_name, domain_build_dir) for sysbuild, or empty list if not sysbuild.
    '''
    domains_yaml = build_dir / 'domains.yaml'
    if not domains_yaml.is_file():
        return []
    try:
        with open(domains_yaml, 'r') as fd:
            domains = yaml.safe_load(fd)
    except yaml.YAMLError as ex:
        raise SbomException(f'Cannot parse "{domains_yaml}".') from ex
    if not isinstance(domains, dict) or 'domains' not in domains:
        raise SbomException(f'Invalid format of "{domains_yaml}".')
    results: list[tuple[str, Path]] = []
    for domain in domains.get('domains', []):
        if not isinstance(domain, dict):
            continue
        name = domain.get('name')
        domain_dir = Path(domain.get('build_dir', ''))
        if not name or not domain_dir:
            raise SbomException(f'Invalid sysbuild domain entry in "{domains_yaml}".')
        if not domain_dir.is_absolute():
            domain_dir = (build_dir / domain_dir).resolve()
        if not domain_dir.exists():
            raise SbomException(f'Detected sysbuild domain "{name}", but the build directory '
                                f'"{domain_dir}" does not exist.')
        results.append((name, domain_dir))
    if len(results) == 0:
        raise SbomException(f'No domains detected in "{domains_yaml}".')
    return results

def get_ninja_default_targets(build_dir: Path) -> list[str]:
    '''
    Returns default targets listed in build.ninja.
    '''
    build_ninja = build_dir / 'build.ninja'
    if not build_ninja.is_file():
        raise SbomException(f'Cannot find "{build_ninja}".')
    with open(build_ninja, 'r') as fd:
        for line in fd:
            if line.startswith('default'):
                parts = line.strip().split(maxsplit=1)
                if len(parts) == 2:
                    targets = parts[1].strip().split()
                    if len(targets) > 0:
                        return targets
                break
    return [DEFAULT_GN_TARGET]


class MapFileItem(DataBaseClass):
    '''
    Holds information about a single entry extracted from a map file.
    Attributes:
        path        Path to linked an object file or a library.
        optional    True if the entry is a linker stuff and don't have to be included in
                    the report.
        content     A dictionary with object file names contained in this library
                    as keys. Values are initially False and they will be changed to True
                    when the file is detected in the build system.
        extracted   Initially False and it will be changed to True when the file is
                    detected in the build system.
    '''
    path: Path = Path()
    optional: bool = False
    content: dict[str, bool] = {}
    extracted: bool = False


def check_external_tools(build_dir: Path):
    '''
    Checks if "ar --version" works correctly. If not, raises exception with information
    for user.
    '''
    def tool_test_execute(tool_path: str) -> bool:
        try:
            command_execute(tool_path, '--version', allow_stderr=True)
            return True
        except: # pylint: disable=bare-except
            # We are checking if calling this command works at all,
            # so ANY kind of problem (exception) should return "False" value.
            return False

    def test_tool(arg_name: str, cmake_var_name: str) -> str:
        if args.__dict__[arg_name] is not None:
            if not tool_test_execute(args.__dict__[arg_name]):
                raise SbomException(f'Cannot execute "{args.__dict__[arg_name]}".\n'
                    f'Make sure you correctly set "--{arg_name}" option.')
            return args.__dict__[arg_name]
        match = re.search(r'^\s*' + cmake_var_name + r'\s*(?::.*?)?=\s*(.*?)\s*$',
                          cmake_cache, re.I | re.M)
        if match is None:
            raise SbomException('Cannot parse "CMakeCache.txt".\n'
                'Make sure that you correctly built the application.')
        result = match.group(1)
        if not tool_test_execute(result):
            raise SbomException(f'Cannot execute "{result}".\n Make sure that your build '
                f'directory is up to date and you correctly configured the build environment.')
        log.dbg(f'"{arg_name}" command detected: {result}')
        return result

    try:
        with open(build_dir / 'CMakeCache.txt', 'r') as fd:
            cmake_cache = fd.read()
    except FileNotFoundError as ex:
        raise SbomException('Cannot find "CMakeCache.txt".\n'
            'Make sure that you correctly built the application.') from ex

    args.ar = test_tool('ar', 'CMAKE_AR')
    args.ninja = test_tool('ninja', 'CMAKE_MAKE_PROGRAM')


def get_default_build_dir() -> str | None:
    '''
    Returns default build directory if exists.
    '''
    default_build_dir = Path(DEFAULT_BUILD_DIR).resolve()
    default_target = default_build_dir / DEFAULT_TARGET
    if default_build_dir.exists() and default_target.exists():
        return DEFAULT_BUILD_DIR
    else:
        return None


def detect_toolchain_version(root: Path) -> str | None:
    '''Read toolchain version detection from known marker files.'''
    for candidate in ('sdk_version', 'version'):
        version_file = root / candidate
        if not version_file.exists():
            continue
        try:
            version = version_file.read_text().strip()
            if version:
                return version
        except OSError as ex:
            log.dbg(f'Cannot read toolchain version file "{version_file}": {ex}')
    return None


def detect_toolchain(build_dir: Path) -> tuple[Path | None, str | None, str | None]:
    '''
    Detect toolchain path, name and version using build_info.yml or CMakeCache.txt.
    Returns (root_path, name, version) or (None, None, None) if not found.
    '''
    info_file = build_dir / 'build_info.yml'
    if info_file.exists():
        try:
            build_info = yaml.safe_load(info_file.read_text()) or dict()
            toolchain = build_info.get('cmake', {}).get('toolchain', {})
            path = toolchain.get('path')
            name = toolchain.get('name')
            if path:
                root = Path(path).expanduser().resolve()
                return (root, name, detect_toolchain_version(root))
        except Exception as ex:  # pylint: disable=broad-except
            log.wrn(f'Cannot parse "{info_file}" for toolchain information: {ex}')

    cache_path = locate_cmake_cache(build_dir)
    if cache_path is not None:
        try:
            cache_content = cache_path.read_text()
            match = re.search(r'^\s*ZEPHYR_SDK_INSTALL_DIR(?:[:A-Z]+)?=(.*)$',
                              cache_content, re.M)
            if match:
                root = Path(match.group(1).strip()).expanduser().resolve()
                return (root, 'zephyr-sdk', detect_toolchain_version(root))
        except OSError as ex:
            log.dbg(f'Cannot read "{cache_path}" for toolchain information: {ex}')

    return (None, None, None)


def register_toolchain_for_build(data: Data, build_dir: Path):
    '''Register detected toolchain as a package and remember its root.'''
    root, name, version = detect_toolchain(build_dir)
    if root is None:
        return
    resolved = root.resolve()
    if not resolved.exists():
        log.dbg(f'Skipping toolchain registration for missing path "{resolved}"')
        return
    resolved_str = str(resolved)
    if resolved_str in data.toolchain_paths:
        return
    package = Package()
    pkg_name = name or resolved.name or 'toolchain'
    pkg_version = version or detect_toolchain_version(resolved) or 'UNKNOWN'
    package.id = f'TOOLCHAIN#{pkg_name.upper()}#{pkg_version.upper()}'
    package.name = pkg_name
    package.url = str(resolved)
    package.version = pkg_version
    if args.package_supplier:
        package.supplier = args.package_supplier
    data.toolchain_paths[resolved_str] = package.id
    data.packages[package.id] = package


def toolchain_package_for_file(data: Data, file_path: Path) -> str | None:
    '''Return toolchain package ID if file_path is inside a known toolchain root.'''
    if not data.toolchain_paths:
        return None
    resolved = file_path.resolve()
    for root_str, package_id in data.toolchain_paths.items():
        try:
            resolved.relative_to(Path(root_str))
            return package_id
        except ValueError:
            continue
    return None


class ExternalToolDetector:
    '''
    Detects external build tools used in the project.
    Commonly they are added as CMake's ExternalProject.
    So far, only GN is detected.
    '''

    GN_MATCHER_RE = re.compile(r'(^|\/|\\)gn(\.exe|\.bat|\.cmd|\.py)?"?$')

    gn_build_directories: set[str]


    def __init__(self):
        '''
        Initialize this instance with empty results.
        '''
        self.gn_build_directories = set()


    def parse_gn_command(self, command: str, current_dir: str):
        '''
        Minimalist GN command parser that extracts directory for the "gn gen" command.
        '''
        try:
            gn_args = shlex.split(command, posix=False)
        except ValueError:
            gn_args = command.split()
        gn_args = [arg.strip('"\'') for arg in gn_args if not arg.strip().startswith('-')]
        if len(gn_args) > 1 and gn_args[1].lower() == 'gen':
            gn_build_dir = '.'
            if len(gn_args) > 2:
                gn_build_dir = gn_args[2]
            gn_path = (Path(current_dir) / gn_build_dir).resolve()
            self.gn_build_directories.add(str(gn_path))


    def parse_command(self, command: str, current_dir: str):
        '''
        Parse a single command to check if it is a GN command.
        '''
        parts = command.split(' ')
        cmd = parts[0].strip()
        if self.GN_MATCHER_RE.search(cmd) is not None:
            self.parse_gn_command(command, current_dir)


    def detect(self, target: str, build_dir: str):
        '''
        Detects known external tools used in the build. Results will be added to the
        "gn_build_directories" property. Detection is based on "ninja -t commands" output
        that lists all the commands executed to fully build the target.
        '''
        lines : str = command_execute(args.ninja, '-t', 'commands', target, cwd=build_dir)
        for line in lines.split('\n'):
            current_dir = build_dir
            # Extract command from Windows cmd.exe /C wrapper if present.
            # Uses string operations instead of regex to avoid ReDoS vulnerability.
            lower_line = line.lower()
            for cmd_pattern in ('cmd.exe /c ', 'cmd /c '):
                cmd_idx = lower_line.find(cmd_pattern)
                if cmd_idx != -1:
                    line = line[cmd_idx + len(cmd_pattern):]
                    if line.startswith('"') and line.endswith('"'):
                        line = line[1:-1]
                    break
            commands = line.split(' && ')
            for command in commands:
                command = command.strip()
                if command.startswith('cd '):
                    if platform.system() == 'Windows':
                        out = command_execute(shutil.which('cmd.exe'), '/C',
                                              f'{command} && cd', cwd=current_dir)
                    else:
                        out = command_execute(f'{command} && pwd', shell=True, cwd=current_dir)
                    for out_line in out.split('\n'):
                        out_line = out_line.strip()
                        if out_line != '':
                            current_dir = out_line
                else:
                    self.parse_command(command, current_dir)


class InputBuild:
    '''
    Class for extracting list of input files for a specific build directory.
    '''

    data: Data | None
    build_dir: Path
    seen_input_files: set[Path]


    def __init__(self, data: Data | None, build_dir: Path):
        '''
        Initialize build directory input object that will fill "data" object and get
        information from "build_dir".
        '''
        self.data = data
        self.build_dir = Path(build_dir)
        self.seen_input_files = set()


    def read_file_list_from_map(self, map_file: Path,
                                items: dict[str, MapFileItem] | None = None
                                ) -> dict[str, MapFileItem]:
        '''
        Parse map file for list of all linked files. The returned dict has absolute resolved path
        as key and value is a MapFileItem.
        '''
        with open(map_file, 'r') as fd:
            map_content = fd.read()
        if items is None:
            items = dict()
        file_entry_re = (r'^(?:[ \t]+\.[^\s]+(?:\r?\n)?[ \t]+0x[0-9a-fA-F]{16}[ \t]+'
                         r'0x[0-9a-fA-F]+|LOAD)[ \t]+(.*?)(?:\((.*)\))?$')
        linker_stuff_re = r'(?:\s+|^)linker\s+|\s+linker(?:\s+|$)'
        lto_markers = ('.ltrans', '.debug.temp', '.lto_priv', '.o')
        for match in re.finditer(file_entry_re, map_content, re.M):
            file = match.group(1)
            file_path = (self.build_dir / file).resolve()
            file_name = Path(file).name
            if str(file_path) not in items:
                exists = file_path.is_file()
                possibly_linker = (match.group(2) is None and
                                   re.search(linker_stuff_re, file, re.I) is not None)
                is_transient = False
                if not exists:
                    if any(file_name.endswith(marker) for marker in lto_markers):
                        is_transient = True
                    if is_transient:
                        log.dbg(f'Skipping temporary build artifact from map file: "{file_path}".')
                optional = ((not exists) and possibly_linker) or is_transient
                if (not exists) and (not optional):
                    raise SbomException(f'The input file {file}, extracted from a map file, '
                                        f'does not exists.')
                content = dict()
                item = MapFileItem()
                item.path = file_path
                item.optional = optional
                item.content = content
                items[str(file_path)] = item
            else:
                item = items[str(file_path)]
                content = item.content
            if match.group(2) is not None:
                file = Path(match.group(2)).name
                content[file] = False
        return items


    def mark_map_archive(self, map_item: MapFileItem, archive: BuildArchive, visited: set = None):
        '''
        Mark entries in "map_item.content" that are in the "archive".
        '''
        file_names: set[str] = set()
        if visited is None:
            visited = set()
        for source in archive.sources.values():
            file_names.add(source.name.lower())
        for obj in archive.objects.values():
            file_names.add(obj.path.name.lower())
            for source in obj.sources.values():
                file_names.add(source.name.lower())
        for archive_entry in map_item.content:
            entry_name = Path(archive_entry).name.lower()
            if entry_name in file_names:
                map_item.content[archive_entry] = True
        for sub in archive.archives.values():
            if sub not in visited:
                visited.add(sub)
                self.mark_map_archive(map_item, sub, visited)


    @staticmethod
    def mark_map_sources(map_items: dict[str, MapFileItem], sources: dict[str, Path]):
        '''
        Mark map items that are in the sources dictionary.
        '''
        for source in sources.values():
            path_str = str(source)
            if path_str in map_items:
                map_items[path_str].extracted = True


    def mark_map_objects(self, map_items: dict[str, MapFileItem], objects: dict[str, BuildObject]):
        '''
        Mark map items that are in the objects dictionary and its sources.
        '''
        for obj in objects.values():
            self.mark_map_sources(map_items, obj.sources)
            path_str = str(obj.path)
            if path_str in map_items:
                map_items[path_str].extracted = True


    def validate_with_map_files(self, inputs: list[BuildArchive], maps: list[str]):
        '''
        Read linked files from a map files and validate if all needed files are
        correctly detected in the "inputs" parameter.
        '''
        from ninja_build_extractor import BuildArchive

        map_items: dict[str, MapFileItem] = dict()

        for map_file in maps:
            self.read_file_list_from_map(map_file, map_items)

        for input in inputs:
            for archive in input.archives.values():
                if str(archive.path) in map_items:
                    map_items[str(archive.path)].extracted = True
                    self.mark_map_archive(map_items[str(archive.path)], archive)
                self.mark_map_sources(map_items, archive.sources)
                self.mark_map_objects(map_items, archive.objects)
            self.mark_map_sources(map_items, input.sources)
            self.mark_map_objects(map_items, input.objects)

        for map_item in map_items.values():
            content_extacted = len(map_item.content) > 0
            for extracted in map_item.content.values():
                if not extracted:
                    content_extacted = False
                    break
            extracted = content_extacted or map_item.extracted
            if (not extracted) and (not map_item.optional):
                if map_item.path.name in args.allowed_in_map_file_only:
                    input = BuildArchive()
                    input.sources[str(map_item.path.name)] = map_item.path
                    inputs.append(input)
                else:
                    raise SbomException(f'Input "{map_item.path}", extracted from a map file, '
                                        'is not detected in a build system.')
            if map_item.extracted and (not content_extacted) and (len(map_item.content) > 0):
                for input in inputs:
                    for archive in input.archives.values():
                        if archive.path == map_item.path:
                            archive.is_leaf = True


    def all_deps_of_archive(self, archive: BuildArchive, visited: set = None) -> set[Path]:
        '''
        Recursively scans the archive and returns a list of all dependencies.
        '''
        if visited is None:
            visited = set()
        all_deps: set[Path] = set(archive.sources.values())
        for obj in archive.objects.values():
            all_deps.add(obj.path)
            all_deps.update(obj.sources.values())
        for sub in archive.archives.values():
            if sub not in visited:
                visited.add(sub)
                all_deps.update(self.all_deps_of_archive(sub, visited))
        return all_deps


    def check_archive_with_ar(self, archive: BuildArchive):
        '''
        Checks if every file in archive is correctly extracted using the "ar" tool.
        '''
        all_deps = set(map(lambda path: path.name, self.all_deps_of_archive(archive)))
        arch_files = command_execute(args.ar, '-t', str(archive.path))
        arch_files = arch_files.split('\n')
        arch_files = (f.strip().replace('/', os.sep).replace('\\', os.sep).strip(os.sep)
                      for f in arch_files)
        arch_files = filter(lambda file: len(file.strip()) > 0, arch_files)
        for arch_file in arch_files:
            if arch_file not in all_deps:
                log.dbg(f'Cannot find "{arch_file}" in dependencies of "{archive.path}"')
                return False
        return True


    def validate_with_ar(self, inputs: list[BuildArchive]):
        '''
        Read list of object files from each archive from "inputs" parameter and compare
        the list with files detected in the build system. If list does not match
        mark a library as a leaf, so it will be listed directly in the report instead of
        its contents.
        '''
        for input in inputs:
            for archive in input.archives.values():
                archive.is_leaf = not self.check_archive_with_ar(archive)


    def add_file_info(self, path: Path):
        '''
        Add output file information using given path.
        If file is empty and it is not a source code, ignore it.
        '''
        if path in self.seen_input_files:
            return
        self.seen_input_files.add(path)
        if (path.stat().st_size > 0) or (path.suffix.lower() in SOURCE_CODE_SUFFIXES):
            file = FileInfo()
            file.file_path = path
            file.file_rel_path = Path(os.path.relpath(path, util.west_topdir()))
            package_id = toolchain_package_for_file(self.data, path)
            if package_id is not None:
                file.package = package_id
            self.data.files.append(file)


    def add_object_info(self, obj: BuildObject):
        '''
        Add output object file information using from "obj". If the object file has
        the sources, they will be added instead of the object file.
        '''
        any_source = False
        for source in obj.sources.values():
            self.add_file_info(source)
            any_source = any_source or (source.suffix.lower() in SOURCE_CODE_SUFFIXES)
        if not any_source:
            self.add_file_info(obj.path)


    @staticmethod
    def merge_inputs(inputs: list[BuildArchive]):
        '''
        Merges all archives from "inputs" parameter that are referring the same file.
        '''
        archives = {}
        for input in inputs:
            for archive_target in list(input.archives.keys()):
                archive = input.archives[archive_target]
                if archive.path in archives:
                    dst = archives[archive.path]
                    archive.merged_with = dst
                    dst.archives.update(archive.archives)
                    dst.objects.update(archive.objects)
                    dst.sources.update(archive.sources)
                    del input.archives[archive_target]
                else:
                    archives[archive.path] = archive
        def update_dict(archive: BuildArchive, visited: set):
            for key in list(archive.archives.keys()):
                sub = archive.archives[key]
                if hasattr(sub, 'merged_with'):
                    archive.archives[key] = sub.merged_with
                if sub not in visited:
                    visited.add(sub)
                    update_dict(sub, visited)
        visited = set()
        for input in inputs:
            update_dict(input, visited)


    def generate(self, targets_with_maps: list[str]):
        '''
        Generate a list of files from specified targets. Targets can optionally have a map
        file name after the ':' sign.
        '''
        from ninja_build_extractor import NinjaBuildExtractor

        # Prepare list of targets and associated map files.
        targets = []
        maps = []
        for target_with_map in targets_with_maps:
            if target_with_map.find(':') >= 0:
                pos = target_with_map.find(':')
                map_file = self.build_dir / target_with_map[(pos + 1):]
                target = target_with_map[:pos]
            else:
                map_file = (self.build_dir / target_with_map).with_suffix('.map')
                target = target_with_map

            if not map_file.exists():
                raise SbomException(f'Cannot find map file for "{target_with_map}" '
                                    f'in build directory "{self.build_dir}". '
                                    f'Expected location "{map_file}".')
            log.dbg(f'Map file {map_file} for target "{target}"')
            targets.append(target)
            maps.append(map_file)
            self.data.inputs.append(f'The "{target}" file from the build directory '
                                    f'"{self.build_dir.resolve()}"')

        # Detect external tools
        ext_detector = ExternalToolDetector()
        for target in targets:
            ext_detector.detect(target, self.build_dir)

        inputs: list[BuildArchive] = []

        # Load list of files from a cache file (debug purposes only)
        if args.debug_build_input_cache is not None:
            try:
                with open(args.debug_build_input_cache, 'rb') as f:
                    inputs = pickle.load(f)
            except: # pylint: disable=bare-except
                # This is a hidden debug-only option, so we can ignore all exceptions
                pass

        # Extract list of files with the NinjaBuildExtractor
        if len(inputs) == 0:
            log.dbg(f'Extracting files from root build directory "{self.build_dir}"')
            root_extractor = NinjaBuildExtractor(self.build_dir)
            inputs.append(root_extractor.extract(targets))
            for directory in ext_detector.gn_build_directories:
                log.dbg(f'Extracting files from GN build directory "{directory}"')
                gn_extractor = NinjaBuildExtractor(directory, True)
                gn_targets = get_ninja_default_targets(Path(directory))
                inputs.append(gn_extractor.extract(gn_targets))
            # Save list of files to a cache file (debug purposes only)
            if args.debug_build_input_cache is not None:
                with open(args.debug_build_input_cache, 'wb') as f:
                    pickle.dump(inputs, f)

        self.merge_inputs(inputs)

        # Do additional validation
        self.validate_with_ar(inputs)
        self.validate_with_map_files(inputs, maps)

        # Pass extracted and validated files as an SBOM input files.
        for input in inputs:
            for archive in input.archives.values():
                if archive.is_leaf:
                    self.add_file_info(archive.path)
                else:
                    for source in archive.sources.values():
                        self.add_file_info(source)
                    for obj in archive.objects.values():
                        self.add_object_info(obj)
            for source in input.sources.values():
                self.add_file_info(source)
            for obj in input.objects.values():
                self.add_object_info(obj)


def generate_input(data: Data):
    '''
    Fill "data" with input files for application build directory specified in the script arguments.
    '''
    if args.build_dir is not None:
        resolved_build_dirs = []
        for build_dir, *targets in args.build_dir:
            resolved_build_dir = get_sysbuild_default_domain(Path(build_dir)) or Path(build_dir)
            if len(targets) == 0:
                targets = [DEFAULT_TARGET]
            resolved_build_dirs.append((build_dir, resolved_build_dir, targets))
        check_external_tools(resolved_build_dirs[0][1])
        for build_dir, resolved_build_dir, targets in resolved_build_dirs:
            register_toolchain_for_build(data, Path(build_dir))
            register_toolchain_for_build(data, resolved_build_dir)
            register_application_root(data, Path(build_dir))
            register_module_roots(data, Path(build_dir))
            log.dbg(f'INPUT: build directory: {build_dir}, '
                    f'resolved build directory: {resolved_build_dir}, targets: {targets}')
            b = InputBuild(data, resolved_build_dir)
            b.generate(targets)


def register_application_root(data: Data, build_dir: Path):
    '''Detect application source root and store as workspace-relative path.'''
    root = detect_application_root(build_dir)
    if root is None:
        return
    workspace = Path(util.west_topdir()).resolve()
    try:
        rel = root.resolve().relative_to(workspace)
    except ValueError:
        return
    rel_str = rel.as_posix()
    if rel_str == '.':
        rel_str = ''
    data.application_roots.add(rel_str)


def register_module_roots(data: Data, build_dir: Path):
    '''Parse module list files in the build directory and store their roots.'''
    for file_name in ('zephyr_modules.txt', 'sysbuild_modules.txt'):
        modules_file = Path(build_dir) / file_name
        if not modules_file.exists():
            continue
        with open(modules_file) as fd:
            for line in fd:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                match = re.fullmatch(r'"([^"]*)":"([^"]*)":"([^"]*)"', line)
                if match is None:
                    continue
                register_module_root_path(data, match.group(2))


def register_module_root_path(data: Data, source: str):
    '''Register module directory if it is inside the west workspace.'''
    source = source.strip()
    if (not source) or source.startswith('${'):
        return
    path = Path(source)
    if not path.is_absolute():
        path = (Path(util.west_topdir()) / path).resolve()
    else:
        path = path.resolve()
    workspace = Path(util.west_topdir()).resolve()
    try:
        rel = path.relative_to(workspace)
    except ValueError:
        return
    rel_str = rel.as_posix()
    if rel_str == '.':
        rel_str = ''
    data.module_roots.add(rel_str)


def detect_application_root(build_dir: Path) -> Path | None:
    '''Try to detect application source directory for provided build directory.'''
    cache_path = locate_cmake_cache(build_dir)
    if cache_path is not None:
        app_dir = parse_application_dir(cache_path)
        if app_dir is not None:
            return app_dir
    parent = build_dir.parent
    return parent if parent.exists() else None


def locate_cmake_cache(build_dir: Path) -> Path | None:
    '''Return best-matching CMakeCache.txt file.'''
    domain_file = build_dir / 'domains.yaml'
    if domain_file.exists():
        try:
            domains = yaml.safe_load(open(domain_file))
            default_domain = domains.get('default')
            if default_domain:
                candidate = build_dir / default_domain / 'CMakeCache.txt'
                if candidate.exists():
                    return candidate
        except Exception: # pylint: disable=broad-except
            pass
    candidate = build_dir / 'CMakeCache.txt'
    return candidate if candidate.exists() else None


def parse_application_dir(cache_path: Path) -> Path | None:
    '''Extract APPLICATION_SOURCE_DIR from CMake cache.'''
    try:
        cache_content = cache_path.read_text()
    except FileNotFoundError:
        return None
    match = re.search(r'^APPLICATION_SOURCE_DIR(?:[:A-Z]+)?=(.*)$', cache_content, re.M)
    if match is not None:
        value = match.group(1).strip()
        if value:
            path = Path(value)
            if path.exists():
                return path
            return path
    return None
