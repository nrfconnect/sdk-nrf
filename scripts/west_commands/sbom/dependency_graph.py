#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''Build dependency evidence, independent of license detection and SPDX serialization.

Paths identify build artifacts until git-info assigns the collected files to packages.
Ninja order-only inputs are retained for discovery, never used as dependency assertions.
'''

from __future__ import annotations

import hashlib
import os
import re
from dataclasses import dataclass, field
from pathlib import Path

from west import log

SOURCE_SUFFIXES = {'.c', '.cpp', '.cxx', '.cc', '.c++', '.s'}
BINARY_SUFFIXES = {'.a', '.lib', '.o', '.obj', '.elf', '.axf'}


def build_path(path: str | Path, directory: Path) -> Path:
    '''Canonical artifact identity, including mixed separators on Windows.'''
    path = Path(path)
    return Path(os.path.normcase(str((directory / path).resolve())))


@dataclass
class BuildNode:
    explicit: set[Path] = field(default_factory=set)
    implicit: set[Path] = field(default_factory=set)
    compiler: set[Path] = field(default_factory=set)
    order_only: set[Path] = field(default_factory=set)
    rule: str = ''

    @property
    def inputs(self) -> set[Path]:
        return self.explicit | self.implicit | self.compiler


@dataclass
class BuildGraph:
    directory: Path
    targets: set[Path] = field(default_factory=set)
    nodes: dict[Path, BuildNode] = field(default_factory=dict)
    # An installed file is an alias for the actual source/build artifact.
    aliases: dict[Path, Path] = field(default_factory=dict)


@dataclass(frozen=True)
class Artifact:
    path: Path
    member: str = ''


@dataclass
class BuildDependencies:
    directory: Path
    source_root: Path | None
    name: str
    targets: set[Path] = field(default_factory=set)
    graphs: list[BuildGraph] = field(default_factory=list)
    files: set[Path] = field(default_factory=set)
    link_references: set[tuple[Artifact, Artifact]] = field(default_factory=set)
    unavailable: set[str] = field(default_factory=set)


def read_link_references(map_file: Path, directory: Path) -> set[tuple[Artifact, Artifact]]:
    '''Read GNU ld's archive-inclusion table, not LOAD/section co-occurrence.

    The table is optional (e.g. lld and LTO). Whole-archive and command-line symbol
    entries have no referring object and therefore cannot establish a package edge.
    '''
    result = set()
    provider = None
    active = False
    member_re = re.compile(r'^(.*?\.(?:a|lib))\(([^()]*)\)\s*(.*)$', re.I)
    consumer_re = re.compile(r'^(.*?)\s+\([^()]+\)$')
    for line in map_file.read_text(errors='replace').splitlines():
        if line.startswith('Archive member included to satisfy reference'):
            active = True
            continue
        if not active:
            continue
        if line and not line[0].isspace():
            match = member_re.match(line)
            if not match:
                break
            provider = Artifact(build_path(match[1], directory), match[2])
            remainder = match[3]
        else:
            remainder = line.strip()
        if not remainder or provider is None:
            continue
        match = consumer_re.match(remainder)
        if match and match[1]:
            consumer_text = match[1]
            member = member_re.match(consumer_text)
            if member:
                consumer = Artifact(build_path(member[1], directory), member[2])
            elif Path(consumer_text).suffix.lower() in {'.o', '.obj'}:
                consumer = Artifact(build_path(consumer_text, directory))
            else:
                provider = None
                continue
            result.add((consumer, provider))
        provider = None
    return result


def read_install_aliases(directory: Path) -> dict[Path, Path]:
    '''Read literal CMake file installs that actually appear in the install manifest.

    Never execute generated CMake scripts. Unsupported expressions, directory installs
    and ambiguous destinations remain unresolved rather than guessing their origin.
    '''
    manifest = directory / 'install_manifest.txt'
    if not manifest.is_file():
        return {}
    installed = {build_path(p, directory) for p in manifest.read_text().splitlines() if p}
    aliases: dict[Path, set[Path]] = {}
    for script in directory.rglob('cmake_install.cmake'):
        text = script.read_text(errors='replace')
        prefix = re.search(r'set\(CMAKE_INSTALL_PREFIX "([^"]+)"\)', text)
        for match in re.finditer(
            r'file\(INSTALL DESTINATION "([^"]+)" TYPE (?:FILE|STATIC_LIBRARY|'
            r'SHARED_LIBRARY|EXECUTABLE) (.*?)\)',
            text,
            re.S,
        ):
            destination = match[1]
            if prefix:
                destination = destination.replace('${CMAKE_INSTALL_PREFIX}', prefix[1])
            if '$' in destination:
                continue
            files = re.search(r'\bFILES\s+(.*)', match[2], re.S)
            if not files:
                continue
            rename = re.search(r'\bRENAME "([^"]+)"', match[2])
            for source in re.findall(r'"([^"]+)"', files[1].split(' RENAME ')[0]):
                if '$' in source:
                    continue
                source_path = build_path(source, directory)
                target = build_path(
                    Path(destination) / (rename[1] if rename else source_path.name), directory
                )
                if target in installed:
                    aliases.setdefault(target, set()).add(source_path)
    return {target: next(iter(sources)) for target, sources in aliases.items() if len(sources) == 1}


def _within(path: Path, directory: Path) -> bool:
    return path == directory or directory in path.parents


class PackageGraph:
    '''Project observed artifact relationships onto detected repository packages.'''

    def __init__(self, data, context: BuildDependencies, application: str):
        self.data = data
        self.context = context
        self.application = application
        self.files = {build_path(f.file_path, context.directory): f for f in data.files}
        self.nodes: dict[Path, BuildNode] = {}
        self.aliases: dict[Path, Path] = {}
        self.owners: dict[Artifact, frozenset[str]] = {}
        self.visiting: set[Artifact] = set()
        self.dependencies: set[tuple[str, str]] = set()
        self.unresolved = set()
        node_origins = {}
        for graph in context.graphs:
            self.aliases.update(graph.aliases)
            reachable = set()
            pending = list(graph.targets)
            while pending:
                path = pending.pop()
                if path in reachable:
                    continue
                reachable.add(path)
                if path in graph.nodes:
                    pending.extend(graph.nodes[path].inputs)
            for path in sorted(reachable):
                node = graph.nodes.get(path)
                if node is None:
                    continue
                # Parent ExternalProject stubs must not hide the real nested rule.
                rank = (
                    bool(node.inputs),
                    _within(path, graph.directory),
                    len(graph.directory.parts),
                    bool(node.rule),
                )
                if path not in node_origins or rank > node_origins[path]:
                    self.nodes[path] = node
                    node_origins[path] = rank

    def owner(self, artifact: Artifact) -> frozenset[str]:
        if artifact in self.owners:
            return self.owners[artifact]
        if artifact in self.visiting:
            return frozenset()
        self.visiting.add(artifact)
        owners = self._owner(artifact)
        self.visiting.remove(artifact)
        self.owners[artifact] = frozenset(owners)
        return self.owners[artifact]

    def _owner(self, artifact: Artifact) -> set[str]:
        path = artifact.path
        if path in self.context.targets and not artifact.member:
            return {self.application}
        if path in self.aliases:
            return set(self.owner(Artifact(self.aliases[path], artifact.member)))
        node = self.nodes.get(path)
        file = self.files.get(path)
        if artifact.member and node:
            # Archive-member basenames alone are insufficient when duplicated.
            members = [
                p for p in node.explicit if p.name == Path(artifact.member.replace('\\', '/')).name
            ]
            if len(members) == 1:
                return set(self.owner(Artifact(members[0])))
            if file is None or not file.package:
                return set()
        # A named repository or toolchain is evidence; the unknown catch-all is not.
        if file is not None and file.package and file.package not in self.data.application_packages:
            return {file.package}
        if node:
            primary = {p for p in node.explicit if p.suffix.lower() in SOURCE_SUFFIXES}
            if primary:
                return set().union(*(self.owner(Artifact(p)) for p in primary))
            if path.suffix.lower() in BINARY_SUFFIXES or node.rule == 'phony':
                return set().union(*(self.owner(Artifact(p)) for p in node.explicit))
        if (
            file is not None
            and file.package in ('', self.application)
            and self.context.source_root
            and _within(path, self.context.source_root)
            and not _within(path, self.context.directory)
        ):
            return {self.application}
        return set()

    def providers(self, path: Path, visiting=None) -> set[str]:
        '''Walk transparent build nodes; stop at package-bearing artifacts.'''
        owners = set(self.owner(Artifact(path)))
        if owners:
            return owners
        visiting = set() if visiting is None else visiting
        if path in visiting:
            return set()
        visiting.add(path)
        node = self.nodes.get(path)
        if node and node.rule == 'phony':
            return set().union(*(self.providers(p, visiting) for p in node.inputs))
        if path.suffix.lower() in BINARY_SUFFIXES | {'.h', '.hpp', '.inc'}:
            self.unresolved.add(path)
        return set()

    def add(self, consumers: set[str], providers: set[str]):
        for consumer in consumers:
            for provider in providers:
                if (
                    consumer != provider
                    and consumer in self.data.packages
                    and provider in self.data.packages
                ):
                    self.dependencies.add((consumer, provider))

    def collect(self):
        for path in self.context.files:
            if path.suffix.lower() in BINARY_SUFFIXES:
                node = self.nodes.get(path)
                if not node or not node.inputs:
                    self.unresolved.add(path)
        for target, node in self.nodes.items():
            primary = {p for p in node.explicit if p.suffix.lower() in SOURCE_SUFFIXES}
            if primary and target.suffix.lower() in {'.o', '.obj'}:
                consumers = set(self.owner(Artifact(target)))
                # Do not assert that every package in a mixed source rule consumes every input.
                if len(consumers) != 1:
                    self.unresolved.add(target)
                    continue
                if not node.compiler:
                    self.unresolved.add(target)
                for dependency in node.inputs - primary:
                    self.add(consumers, self.providers(dependency))
            if target in self.context.targets:
                for dependency in node.inputs:
                    self.add({self.application}, self.providers(dependency))
            elif target.suffix.lower() in {'.a', '.lib', '.elf', '.axf'}:
                # Archives containing other archives establish a dependency only when
                # their own source objects unambiguously identify the consumer.
                own_objects = [p for p in node.explicit if p.suffix.lower() in {'.o', '.obj'}]
                consumers = set().union(*(self.owner(Artifact(p)) for p in own_objects))
                if len(consumers) == 1:
                    for dependency in node.inputs:
                        if dependency.suffix.lower() in {'.a', '.lib'}:
                            self.add(consumers, self.providers(dependency))
        for consumer, provider in self.context.link_references:
            consumers = set(self.owner(consumer))
            providers = set(self.owner(provider))
            if len(consumers) == 1 and len(providers) == 1:
                self.add(consumers, providers)
            else:
                self.unresolved.add(provider.path)
        for consumer, provider in sorted(self.dependencies):
            package = self.data.packages[consumer]
            package.dependencies = sorted(set(package.dependencies) | {provider})
        missing = len(self.unresolved)
        if missing or self.context.unavailable:
            reasons = ', '.join(sorted(self.context.unavailable))
            log.wrn(
                f'Dependency graph for "{self.context.name}" is partial: '
                f'{missing} unresolved build artifacts'
                f'{("; " + reasons) if reasons else ""}. Unresolved dependencies are omitted.'
            )


def resolve_dependencies(data):
    '''Run after package detection, before output preprocessing and serialization.'''
    from data_structure import Package

    contexts = data.build_dependencies
    names = [context.name for context in contexts]
    for context in contexts:
        # Preserve the existing single-build application catch-all when it is present.
        use_catchall = len(contexts) == 1 and any(
            not file.package and build_path(file.file_path, context.directory) in context.files
            for file in data.files
        )
        key = (
            ''
            if use_catchall
            else 'BUILD#' + hashlib.sha256(str(context.directory).encode('utf-8')).hexdigest()[:16]
        )
        if key not in data.packages:
            package = Package()
            package.id = key
            data.packages[key] = package
        data.application_packages.add(key)
        data.packages[key].name = (
            f'{context.name} ({context.directory.name})'
            if names.count(context.name) > 1
            else context.name
        )
        for file in data.files:
            path = build_path(file.file_path, context.directory)
            # Shared unknown files cannot establish ownership by either application.
            if (
                not file.package
                and path in context.files
                and sum(path in other.files for other in contexts) == 1
            ):
                file.package = key
        graph = PackageGraph(data, context, key)
        graph.collect()
