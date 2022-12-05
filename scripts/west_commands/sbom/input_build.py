#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Get input files from an application build directory.
'''

import os
import re
from enum import Enum
from pathlib import Path
from types import SimpleNamespace
from west import log, util
from args import args
from data_structure import Data, FileInfo
from common import SbomException, command_execute


DEFAULT_BUILD_DIR = 'build'
DEFAULT_TARGET = 'zephyr/zephyr.elf'


class FileType(Enum):
    '''Defines type of files that are significant for this module.'''
    ARCHIVE = 'archive'
    OBJ = 'obj'
    OTHER = 'other'


def detect_file_type(file: Path) -> FileType:
    '''Simple detector for type of a file based on its header.'''
    with open(file, 'r', encoding='8859') as fd:
        header = fd.read(16)
    if header.startswith('!<arch>\n'):
        return FileType.ARCHIVE
    elif header.startswith('\x7FELF'):
        return FileType.OBJ
    else:
        return FileType.OTHER


class InputBuild:
    '''
    Class for extracting list of input files for a specific build direcory.
    '''

    data: Data
    build_dir: Path
    deps: 'dict[set[str]]'


    def __init__(self, data: Data, build_dir: Path):
        '''
        Initialize build directory input object that will fill "data" object and get
        information from "build_dir".
        '''
        self.map_items = None
        self.data = data
        self.build_dir = Path(build_dir)
        deps_file_name = command_execute(args.ninja, '-t', 'deps', cwd=self.build_dir,
                                         return_path=True)
        self.parse_deps_file(deps_file_name)


    def parse_deps_file(self, deps_file_name: str):
        '''
        Reads all dependencies stored in the .ninja_deps file and sores it in self.deps dictionary.
        '''
        self.deps = dict()
        target_line_re = re.compile(r'([^\s]+)\s*:\s*(#.*)?')
        dep_line_re = re.compile(r'\s+(.*?)\s*(#.*)?')
        empty_line_re = re.compile(r'\s*(#.*)?')
        with open(deps_file_name, 'r') as fd:
            line_no = 0
            while True:
                line = fd.readline()
                line_no += 1
                if len(line) == 0:
                    break
                line = line.rstrip()
                m = target_line_re.fullmatch(line)
                if m is not None:
                    dep = set()
                    self.deps[m.group(1)] = dep
                    continue
                m = dep_line_re.fullmatch(line)
                if m is not None:
                    dep.add(m.group(1))
                    continue
                m = empty_line_re.fullmatch(line)
                if m is None:
                    raise SbomException(f'Cannot parse ninja dependencies output '
                                        f'"{deps_file_name}" on line {line_no}!')


    def query_inputs(self, target: str) -> 'tuple[set[str], set[str], set[str], bool]':
        '''
        Parse output of "ninja -t query <target>" command to find out all input targets.
        The result is a tuple containing:
            - set of explicit inputs
            - set of implicit inputs
            - set of "order only" inputs
            - bool set to True if provided target is a "phony" target
        '''
        lines = command_execute(args.ninja, '-t', 'query', target, cwd=self.build_dir)
        ex_begin = f'Cannot parse output of "{args.ninja} -t query target" on line'
        lines = lines.split('\n')
        lines = tuple(filter(lambda line: len(line.strip()) > 0, lines))
        line_no = 0
        explicit = set()
        implicit = set()
        order_only = set()
        phony = False
        while line_no < len(lines):
            m = re.fullmatch(r'\S.*:', lines[line_no])
            if m is None:
                raise SbomException(f'{ex_begin} {line_no + 1}. Expecting target.')
            line_no += 1
            while line_no < len(lines):
                m = re.fullmatch(r'(\s*)(.*):(.*)', lines[line_no])
                if m is None:
                    raise SbomException(f'{ex_begin} {line_no + 1}. Expecting direction.')
                if m.group(1) == '':
                    break
                line_no += 1
                ind1 = len(m.group(1))
                dir = m.group(2)
                phony = phony or (re.search(r'(\s|^)phony(\s|$)', m.group(3)) is not None)
                if dir == 'input':
                    inputs = True
                else:
                    if dir != 'outputs':
                        raise SbomException(f'{ex_begin} {line_no + 1}. Expecting "input:" '
                                            f'or "outputs:".')
                    inputs = False
                while line_no < len(lines):
                    m = re.fullmatch(r'(\s*)(\|?\|?)\s*(.*)', lines[line_no])
                    if m is None:
                        raise SbomException(f'{ex_begin} {line_no + 1}. Expecting {dir} target.')
                    if len(m.group(1)) <= ind1:
                        break
                    line_no += 1
                    target = str(m.group(3))
                    if inputs:
                        if m.group(2) == '':
                            explicit.add(target)
                        elif m.group(2) == '|':
                            implicit.add(target)
                        else:
                            order_only.add(target)
        return (explicit, implicit, order_only, phony)


    def query_inputs_recursive(self, target: str, done: 'set|None' = None,
                               inputs_tuple=None) -> 'set[str]':
        '''
        Reads recursively set of all input targets for specified "target".
        Optional set "done" contains all targets that are already scanned. It will be updated.
        If you have already result of query_inputs(target), then you can pass it
        to "inputs_tuple" to avoid repeating time consuming operations.
        '''
        if done is None:
            done = set()
        if inputs_tuple is None:
            explicit, implicit, _, _ = self.query_inputs(target)
        else:
            explicit, implicit, _, _ = inputs_tuple
        inputs = explicit.union(implicit)
        result = set()
        for input in inputs:
            file_path = (self.build_dir / input).resolve()
            if input in done:
                continue
            done.add(input)
            if file_path.exists():
                result.add(input)
            else:
                sub_inputs_tuple = self.query_inputs(input)
                phony = sub_inputs_tuple[3]
                if not phony:
                    raise SbomException(f'The input "{input}" does not exist or '
                                        f'it is invalid build target.')
                sub_result = self.query_inputs_recursive(input, done, sub_inputs_tuple)
                result.update(sub_result)
        return result


    def read_file_list_from_map(self, map_file: Path) -> 'dict()':
        '''
        Parse map file for list of all linked files. The returned dict has absolute resolved path
        as key and value is a SimpleNamespace. It contains:
            - path: Path      - Path to linked an object file or a library
            - optional: bool  - True if the entry is a linker stuff and don't have
                                to be included in the report
            - content: dict() - A dictionary with object file names contained in this library
                                as keys. Value is always False, it will be changed to True when
                                the file is extracted from the build system.
            - extracted: bool - Always False. It will be changed to True when the file is extracted
                                from the build system.
        '''
        with open(map_file, 'r') as fd:
            map_content = fd.read()
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
                item = SimpleNamespace(
                    path=file_path,
                    optional=(not exists) and possibly_linker,
                    content=content,
                    extracted=False)
                items[str(file_path)] = item
            else:
                item = items[str(file_path)]
                content = item.content
            if match.group(2) is not None:
                file = Path(match.group(2)).name
                content[file] = False
        return items


    @staticmethod
    def verify_archive_inputs(archive_path: Path, inputs: 'set(str)') -> bool:
        '''
        Checks if every file from the library "archive_path" is in the set of "inputs" targets.
        '''
        arch_files = command_execute(args.ar, '-t', archive_path)
        arch_files = arch_files.split('\n')
        arch_files = (f.strip().replace('/', os.sep).replace('\\', os.sep).strip(os.sep)
                      for f in arch_files)
        arch_files = filter(lambda file: len(file.strip()) > 0, arch_files)
        for arch_file in arch_files:
            for input in inputs:
                if input == arch_file:
                    break
                if input.endswith(os.sep + arch_file):
                    break
            else:
                return False
        return True


    def process_archive(self, archive_path: Path, archive_target: str) -> 'set(Path)':
        '''
        Return set of dependencies of the "archive_target" target which is a library
        file at "archive_path". Return "archive_path" if we cannot find any dependencies.
        It also checks if list of direct dependencies is the same as extracted from
        the library using "ar" tool. It marks this file and its content at the list
        from map file as extracted.
        '''
        if str(archive_path) in self.map_items:
            map_item = self.map_items[str(archive_path)]
            map_item.extracted = True
        else:
            log.wrn(f'Target depends on archive "{archive_path}", but it is not in a map file.')
            map_item = None
        archive_inputs = self.query_inputs_recursive(archive_target)
        if not self.verify_archive_inputs(archive_path, archive_inputs):
            if map_item is not None:
                map_item.content = dict()
            return {archive_path}
        leafs = set()
        for input in archive_inputs:
            input_path = (self.build_dir / input).resolve()
            input_type = detect_file_type(input_path)
            if map_item is not None:
                if input_path.name in map_item.content:
                    map_item.content[input_path.name] = True
            if input_type == FileType.OTHER:
                leafs.add(input_path)
            else:
                if input_type != FileType.OBJ:
                    raise SbomException(f'File "{input_path}" contained in "{archive_path}" is '
                                        f'not an object file.')
                leafs.update(self.process_obj(input_path, input))
        return leafs


    def process_obj(self, input_path: Path, input_target: str) -> 'set(Path)':
        '''
        Return set of dependencies of the "input_target" target which is a object
        file at "input_path". Return "input_path" if we cannot find any dependencies.
        '''
        if input_target not in self.deps:
            return {input_path}
        deps = self.deps[input_target]
        deps = deps.union(self.query_inputs_recursive(input_target))
        return set((self.build_dir / x).resolve() for x in deps)


    def generate_from_target(self, target: str):
        '''
        Generate list of files from a build directory used to create a specific target (elf file).
        The 'target' parameter can contains ':' followed by the custom map file path.
        Also, verifies that list from a map file is fully covered by the build system.
        '''
        if target.find(':') >= 0:
            pos = target.find(':')
            map_file = self.build_dir / target[(pos + 1):]
            target = target[:pos]
        else:
            map_file = (self.build_dir / target).with_suffix('.map')

        if not map_file.exists():
            raise SbomException(f'Cannot find map file for "{target}" '
                                f'in build directory "{self.build_dir}". '
                                f'Expected location "{map_file}".')
        log.dbg(f'Map file: {map_file}')

        self.map_items = self.read_file_list_from_map(map_file)

        self.data.inputs.append(f'The "{target}" file from the build directory '
                                f'"{self.build_dir.resolve()}"')
        elf_inputs = self.query_inputs_recursive(target)
        leafs = set()
        for input in elf_inputs:
            input_path = (self.build_dir / input).resolve()
            input_type = detect_file_type(input_path)
            if input_type == FileType.ARCHIVE:
                leafs.update(self.process_archive(input_path, input))
            else:
                if str(input_path) in self.map_items:
                    item = self.map_items[str(input_path)]
                    item.extracted = True
                    item.content = dict()
                if input_type == FileType.OBJ:
                    leafs.update(self.process_obj(input_path, input))
                else:
                    leafs.add(input_path)

        valid = True
        for name, item in self.map_items.items():
            if item.path.name in args.allowed_in_map_file_only:
                leafs.add(item.path)
                item.extracted = True
                item.content = dict()
            if (not item.extracted) and (not item.optional):
                valid = False
                log.err(f'Input "{name}", extracted from a map file, is not detected in a '
                        f'build system.')
            for file, value in item.content.items():
                if not value:
                    valid = False
                    log.err(f'File "{file}" from "{name}", extracted from a map file, '
                            f'is not detected in a build system.')
        if not valid:
            raise SbomException('Detected differences between a map file and a build system.')

        for leaf in leafs:
            file = FileInfo()
            file.file_path = leaf
            file.file_rel_path = Path(os.path.relpath(leaf, util.west_topdir()))
            self.data.files.append(file)


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
    '''Returns default build directory if exists.'''
    default_build_dir = Path(DEFAULT_BUILD_DIR).resolve()
    default_target = default_build_dir / DEFAULT_TARGET
    if default_build_dir.exists() and default_target.exists():
        return DEFAULT_BUILD_DIR
    else:
        return None


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
            for target in targets:
                b.generate_from_target(target)
