#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Get input files from an application build directory.
'''

import re
import time
from enum import Enum
from pathlib import Path
from threading import Lock
from west import log
from args import args
from data_structure import DataBaseClass
from common import SbomException, command_execute, concurrent_pool_iter, get_executor_workers


COMMAND_LINE_MAX_SIZE = 7000 # Size of command line parameters in Windows is limited.


class BuildObject(DataBaseClass):
    '''
    Holds information about a single object file.
    (internal) Special instance of this class with `path` set to `None` is a placeholder
    for all source files that are not inside any particular object file.
    '''
    path: 'Path | None' = None
    parent: 'BuildArchive'
    sources: 'dict[str, Path]' = {}


class BuildArchive(DataBaseClass):
    '''
    Holds information about a single archive file (library).
    Special instance of this class with `path` set to `None` is a build
    root that contains all source and object files that are not inside
    any particular archive and ALL archive files.
    (internal) The `objects` dictionary item with key "" (empty string) is a placeholder
    for all source files that are not inside any particular object file.
    '''
    path: 'Path | None' = None
    archives: 'dict[str, BuildArchive]' = {}
    objects: 'dict[str, BuildObject]' = {}
    sources: 'dict[str, Path]' = {}


class FileType(Enum):
    '''Defines type of files that are significant for this module.'''
    ARCHIVE = 'archive'
    OBJ = 'obj'
    STAMP = 'stamp'
    MISSING = 'missing'
    OTHER = 'other'


class NinjaBuildExtractor:
    '''
    Class for extracting list of input files for a specific ninja build direcory.
    '''

    build_dir: Path
    include_order_only: bool
    deps: 'dict[set[str]]'

    archives: 'list[BuildArchive]'
    process_target_queue: 'list[tuple[str, BuildObject, BuildArchive]]'
    cache: 'dict[str, set[str]]'
    query_cache: 'dict[str, tuple[set[str], set[str], set[str], bool]]'
    file_type_cache: 'dict[str, FileType]'
    lock: Lock
    target_details: 'dict[str, tuple[Path, FileType]]'
    root: BuildArchive
    archives: 'list[BuildArchive]'


    def __init__(self, build_dir: Path, include_order_only=False):
        '''
        Initialize build directory input object that will fill "data" object and get
        information from "build_dir".
        '''
        self.process_target_queue = list()
        self.cache = dict()
        self.query_cache = dict()
        self.file_type_cache = dict()
        self.build_dir = Path(build_dir)
        self.include_order_only = include_order_only
        self.lock = Lock()
        self.target_details = dict()
        self.root = BuildArchive()
        self.root.path = None
        dummy_object = BuildObject()
        dummy_object.parent = self.root
        self.root.objects[''] = dummy_object
        self.archives = []
        deps_file_name = command_execute(args.ninja, '-t', 'deps', cwd=self.build_dir,
                                         return_path=True)
        self.parse_deps_file(deps_file_name)


    def detect_file_type(self, file: Path) -> FileType:
        '''
        Simple detector for type of a file based on its header.
        '''
        str_file_name = str(file)
        if str_file_name in self.file_type_cache:
            return self.file_type_cache[str_file_name]
        if not file.exists():
            self.file_type_cache[str_file_name] = FileType.MISSING
            return FileType.MISSING
        with open(file, 'r', encoding='8859') as fd:
            header = fd.read(16)
        if header.startswith('!<arch>\n'):
            self.file_type_cache[str_file_name] = FileType.ARCHIVE
            return FileType.ARCHIVE
        elif header.startswith('\x7FELF'):
            self.file_type_cache[str_file_name] = FileType.OBJ
            return FileType.OBJ
        elif file.suffix.lower() == '.stamp':
            self.file_type_cache[str_file_name] = FileType.STAMP
            return FileType.STAMP
        else:
            self.file_type_cache[str_file_name] = FileType.OTHER
            return FileType.OTHER


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


    def prefetch_query_inputs(self, targets: 'list[str]'):
        '''
        Execute "ninja -t query <targets...>" and cache multiple targets to speed up
        future queries. Querying for multiple targets at once is much faster than one at a time.
        '''
        query_args = [args.ninja, '-t', 'query']
        query_len = len(' '.join(query_args))
        for i in range(len(targets)): # pylint: disable=consider-using-enumerate
            query_args.append(targets[i])
            query_len += len(targets[i]) + 1
            if (i == len(targets) - 1) or (query_len >= COMMAND_LINE_MAX_SIZE):
                self.lock.release()
                try:
                    lines = command_execute(*query_args, cwd=self.build_dir)
                finally:
                    self.lock.acquire()
                self.parse_query_output(lines)
                query_args = [args.ninja, '-t', 'query']
                query_len = len(' '.join(query_args))


    def query_inputs(self, target: str) -> 'tuple[set[str], set[str], set[str], bool]':
        '''
        Parse output of "ninja -t query <target>" command to find out all input targets.
        The result is a tuple containing:
            - set of explicit inputs
            - set of implicit inputs
            - set of "order only" inputs
            - bool set to True if provided target is a "phony" target
        '''
        if target in self.query_cache:
            return self.query_cache[target]
        self.lock.release()
        try:
            lines = command_execute(args.ninja, '-t', 'query', target, cwd=self.build_dir)
        finally:
            self.lock.acquire()
        self.parse_query_output(lines)
        if target not in self.query_cache:
            raise SbomException(f'Invalid output of "{args.ninja} -t query"')
        return self.query_cache[target]


    def parse_query_output(self, lines: 'list[str]'):
        ex_begin = f'Cannot parse output of "{args.ninja} -t query" on line'
        lines = lines.split('\n')
        lines = tuple(filter(lambda line: len(line.strip()) > 0, lines))
        line_no = 0
        while line_no < len(lines):
            m = re.fullmatch(r'(\S.*)\s*:', lines[line_no])
            if m is None:
                raise SbomException(f'{ex_begin} {line_no + 1}. Expecting target.')
            target = m.group(1)
            explicit = set()
            implicit = set()
            order_only = set()
            phony = False
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
                    input_target = str(m.group(3))
                    if inputs:
                        if m.group(2) == '':
                            explicit.add(input_target)
                        elif m.group(2) == '|':
                            implicit.add(input_target)
                        else:
                            order_only.add(input_target)
            self.query_cache[target] = (explicit, implicit, order_only, phony)


    def query_inputs_recursive(self, target: str, done: 'set|None' = None,
                               inputs_tuple=None) -> 'set[str]':
        '''
        Reads recursively set of all input targets for specified "target".
        Optional set "done" contains all targets that are already scanned. It will be updated.
        If you have already result of query_inputs(target), then you can pass it
        to "inputs_tuple" to avoid repeating time consuming operations.
        '''
        if target in self.cache:
            while self.cache[target] is None:
                self.lock.release()
                time.sleep(0.1)
                self.lock.acquire()
            return self.cache[target]
        self.cache[target] = None
        if done is None:
            done = set()
        if inputs_tuple is None:
            explicit, implicit, order_only, _ = self.query_inputs(target)
        else:
            explicit, implicit, order_only, _ = inputs_tuple
        inputs = explicit.union(implicit)
        if self.include_order_only:
            inputs = inputs.union(order_only)
        result = set()
        if target in self.deps:
            result.update(self.deps[target])
        for input in inputs:
            file_path = (self.build_dir / input).resolve()
            if input in done:
                continue
            done.add(input)
            file_type = self.detect_file_type(file_path)
            if file_type == FileType.STAMP:
                sub_result = self.query_inputs_recursive(input, done)
                result.update(sub_result)
            elif file_type != FileType.MISSING:
                result.add(input)
            else:
                sub_inputs_tuple = self.query_inputs(input)
                phony = sub_inputs_tuple[3]
                if not phony:
                    log.wrn(f'The input "{input}" does not exist or it is invalid build target.')
                sub_result = self.query_inputs_recursive(input, done, sub_inputs_tuple)
                result.update(sub_result)
        self.cache[target] = result
        return result


    def process_target_delayed(self, target: str, obj: BuildObject, direct_parent_archive: 'BuildArchive | None'):
        '''
        Calls self.process_target(), but if it potentially requires execution of external process,
        postpone it by putting it into the queue.
        '''
        if (target in self.cache) and (self.cache[target] is not None):
            self.process_target(target, obj, direct_parent_archive)
            return
        self.process_target_queue.append((target, obj, direct_parent_archive))


    def process_target_inputs(self, target: str, obj: BuildObject, direct_parent_archive: 'BuildArchive | None'):
        '''
        Calls self.process_target_delayed() for each detected target input.
        '''
        inputs = self.query_inputs_recursive(target)
        for input in inputs:
            self.process_target_delayed(input, obj, direct_parent_archive)


    def get_target_details(self, target: str) -> 'tuple[Path, FileType]':
        '''
        Returns details of specific target - tuple containing:
            - a full resolved target Path
            - a target type from FileType enum
        The results are cached in 'self.target_details'.
        '''
        if target not in self.target_details:
            target_path = (self.build_dir / target).resolve()
            target_type = self.detect_file_type(target_path)
            self.target_details[target] = (target_path, target_type)
        return self.target_details[target]


    def process_target(self, target: str, obj: BuildObject, direct_parent_archive: 'BuildArchive | None'):
        '''
        Process a single target which means adding the target to appropriate place in the results and
        processing all dependent targets.
        Parameters:
            target:                a target name
            obj:                   a build object file which is a parent of this target
            direct_parent_archive: if not None, this target depends directly on it
        '''
        target_path, target_type = self.get_target_details(target)
        if target_type == FileType.ARCHIVE:
            if target in self.archives:
                new_archive = self.archives[target]
            else:
                new_archive = BuildArchive()
                new_archive.path = target_path
                dummy_object = BuildObject()
                dummy_object.parent = new_archive
                new_archive.objects[''] = dummy_object
                self.archives[target] = new_archive
                self.process_target_inputs(target, dummy_object, new_archive)
            if direct_parent_archive is not None:
                direct_parent_archive.archives[target] = new_archive
        elif target_type == FileType.OBJ:
            archive = obj.parent
            if target in archive.objects:
                return
            new_obj = BuildObject()
            new_obj.path = target_path
            new_obj.parent = archive
            archive.objects[target] = new_obj
            self.process_target_inputs(target, new_obj, None)
        elif target_type == FileType.OTHER:
            archive = obj.parent
            if target in obj.sources:
                return
            obj.sources[target] = target_path
            self.process_target_inputs(target, obj, None)
        else:
            raise ValueError('Invalid input_type.')


    @staticmethod
    def remove_dummy_objects(archives: 'dict[str, BuildArchive]'):
        '''
        Internally all files that does not depend on any object file are
        placed in dummy object with path None and empty string as a key.
        This function moves them into the sources field of a BuildArchive.
        '''
        for archive in archives.values():
            if '' in archive.objects:
                archive.sources.update(archive.objects[''].sources)
                del archive.objects['']


    def prefetch_targets(self, targets: 'list[str]'):
        '''
        Calls "prefetch_query_inputs" function on provided list of targets using multiple
        threads.
        '''
        def prefetch_chunk(exec_args):
            with self.lock:
                self.prefetch_query_inputs(exec_args)
        executor_workers = get_executor_workers()
        targets = list(filter(lambda target: target not in self.query_cache, set(targets)))
        if len(targets) <= 4 * executor_workers:
            self.prefetch_query_inputs(targets)
        else:
            chunk_size = (len(targets) + executor_workers - 1) // executor_workers
            chunks = [targets[i:i + chunk_size] for i in range(0, len(targets), chunk_size)]
            self.lock.release()
            try:
                for _ in concurrent_pool_iter(prefetch_chunk, chunks):
                    pass
            finally:
                self.lock.acquire()


    def extract(self, targets: 'list[str]') -> BuildArchive:
        '''
        Returns a build archive that contains all the input files extracted from
        the specified targets. The returned value is in a BuildArchive object.
        '''
        def process_target_execute(exec_args):
            with self.lock:
                self.process_target(*exec_args)

        with self.lock:
            self.process_target_queue = list()
            self.archives = dict()
            for target in targets:
                inputs = self.query_inputs_recursive(target)
                for input in inputs:
                    self.process_target_delayed(input, self.root.objects[''], None)
            while len(self.process_target_queue) > 0:
                queue = self.process_target_queue
                self.process_target_queue = list()
                self.prefetch_targets(list(map(lambda item: item[0], queue)))
                self.lock.release()
                try:
                    for _ in concurrent_pool_iter(process_target_execute, queue):
                        pass
                finally:
                    self.lock.acquire()
            self.remove_dummy_objects(self.archives)
            self.remove_dummy_objects({ '': self.root })
            self.root.archives.update(self.archives)

        return self.root
