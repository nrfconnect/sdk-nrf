#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Get input files from an application build directory.
'''

import os
import pickle
import platform
import re
import shutil
from pathlib import Path
from west import log, util
from args import args
from data_structure import Data, FileInfo, DataBaseClass
from common import SbomException, command_execute
from ninja_build_extractor import BuildArchive, NinjaBuildExtractor, BuildObject


DEFAULT_BUILD_DIR = 'build'
DEFAULT_TARGET = 'zephyr/zephyr.elf'
DEFAULT_GN_TARGET = 'default'
SOURCE_CODE_SUFFIXES = ['.c', '.cpp', '.cxx', '.cc', '.c++', '.s']


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
    content: 'dict[str, bool]' = {}
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
        match = re.search(r'^\s*' + cmake_var_name + r'\s*(?::.*?)?=\s*(.*?)\s*$', cmake_cache,
                          re.I | re.M)
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


def get_default_build_dir() -> 'str|None':
    '''
    Returns default build directory if exists.
    '''
    default_build_dir = Path(DEFAULT_BUILD_DIR).resolve()
    default_target = default_build_dir / DEFAULT_TARGET
    if default_build_dir.exists() and default_target.exists():
        return DEFAULT_BUILD_DIR
    else:
        return None


class ExternalToolDetector:
    '''
    Detects external build tools used in the project.
    Commonly they are added as CMake's ExternalProject.
    So far, only GN is detected.
    '''

    GN_MATCHER_RE = re.compile(r'(^|\/|\\)gn(\.exe|\.bat|\.cmd|\.py)?"?$')
    ARGS_MATCHER = re.compile(r'(?:[\'"](?:\\.|.)*?[\'"]|[^\s])+')

    gn_build_directories: 'set[str]'


    def __init__(self):
        '''
        Initialize this instance with empty results.
        '''
        self.gn_build_directories = set()


    def parse_gn_command(self, command: str, current_dir: str):
        '''
        Minimalists GN command parser that extracts directory for the "gn gen" command.
        '''
        gn_args :'list[str]' = []
        for arg in self.ARGS_MATCHER.finditer(command):
            arg = arg.group(0).strip('"\'')
            if arg.startswith('-'):
                continue
            gn_args.append(arg)
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
            if re.match(r'cmd(\.exe)?\s+\/C\s+', line, re.IGNORECASE):
                line = re.sub(r'^cmd(?:\.exe)?\s+\/C\s+(?:"(.+)"|(.+))$', r'\1\2', line, 0, re.IGNORECASE)
            commands = line.split(' && ')
            for command in commands:
                command = command.strip()
                if command.startswith('cd '):
                    if platform.system() == 'Windows':
                        out = command_execute(shutil.which('cmd.exe'), '/C', f'{command} && cd', cwd=current_dir)
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

    data: 'Data | None'
    build_dir: Path


    def __init__(self, data: 'Data | None', build_dir: Path):
        '''
        Initialize build directory input object that will fill "data" object and get
        information from "build_dir".
        '''
        self.data = data
        self.build_dir = Path(build_dir)


    def read_file_list_from_map(self, map_file: Path,
                                items: 'dict[str, MapFileItem]|None' = None
                                ) -> 'dict[str, MapFileItem]':
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
        for match in re.finditer(file_entry_re, map_content, re.M):
            file = match.group(1)
            file_path = (self.build_dir / file).resolve()
            if str(file_path) not in items:
                exists = file_path.is_file()
                possibly_linker = (match.group(2) is None and
                                   re.search(linker_stuff_re, file, re.I) is not None)
                if (not exists) and (not possibly_linker):
                    raise SbomException(f'The input file {file}, extracted from a map file, '
                                        f'does not exists.')
                content = dict()
                item = MapFileItem()
                item.path = file_path
                item.optional = possibly_linker and not exists
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
        file_names: 'set[str]' = set()
        if visited is None:
            visited = set()
        for source in archive.sources.values():
            file_names.add(source.name.lower())
        for obj in archive.objects.values():
            file_names.add(obj.path.name.lower())
            for source in obj.sources.values():
                file_names.add(source.name.lower())
        for archive_entry in map_item.content.keys():
            entry_name = Path(archive_entry).name.lower()
            if entry_name in file_names:
                map_item.content[archive_entry] = True
        for sub in archive.archives.values():
            if sub not in visited:
                visited.add(sub)
                self.mark_map_archive(map_item, sub, visited)


    @staticmethod
    def mark_map_sources(map_items: 'dict[str, MapFileItem]', sources: 'dict[str, Path]'):
        '''
        Mark map items that are in the sources dictionary.
        '''
        for source in sources.values():
            path_str = str(source)
            if path_str in map_items:
                map_items[path_str].extracted = True


    def mark_map_objects(self, map_items: 'dict[str, MapFileItem]', objects: 'dict[str, BuildObject]'):
        '''
        Mark map items that are in the objects dictionary and its sources.
        '''
        for obj in objects.values():
            self.mark_map_sources(map_items, obj.sources)
            path_str = str(obj.path)
            if path_str in map_items:
                map_items[path_str].extracted = True


    def validate_with_map_files(self, inputs: 'list[BuildArchive]', maps: 'list[str]'):
        '''
        Read linked files from a map files and validate if all needed files are
        correctly detected in the "inputs" parameter.
        '''
        map_items: 'dict[str, MapFileItem]' = dict()

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
            content_extacted = (len(map_item.content) > 0)
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


    def all_deps_of_archive(self, archive: BuildArchive, visited: set = None) -> 'set[Path]':
        '''
        Recursively scans the archive and returns a list of all dependencies.
        '''
        if visited is None:
            visited = set()
        all_deps: 'set[Path]' = set(archive.sources.values())
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


    def validate_with_ar(self, inputs: 'list[BuildArchive]'):
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
        if (path.stat().st_size > 0) or (path.suffix.lower() in SOURCE_CODE_SUFFIXES):
            file = FileInfo()
            file.file_path = path
            file.file_rel_path = Path(os.path.relpath(path, util.west_topdir()))
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
    def merge_inputs(inputs: 'list[BuildArchive]'):
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


    def generate(self, targets_with_maps: 'list[str]'):
        '''
        Generate a list of files from specified targets. Targets can optionally have a map
        file name after the ':' sign.
        '''
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

        inputs: 'list[BuildArchive]' = []

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
                inputs.append(gn_extractor.extract([DEFAULT_GN_TARGET]))
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
        log.wrn('Fetching input files from a build directory is experimental for now.')
        check_external_tools(Path(args.build_dir[0][0]))
        for build_dir, *targets in args.build_dir:
            if len(targets) == 0:
                targets = [DEFAULT_TARGET]
            log.dbg(f'INPUT: build directory: {build_dir}, targets: {targets}')
            b = InputBuild(data, build_dir)
            b.generate(targets)
