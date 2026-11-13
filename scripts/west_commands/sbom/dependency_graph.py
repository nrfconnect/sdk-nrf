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
    '''Normalize artifact paths without looking up files or following symbolic links.'''
    return Path(os.path.normcase(os.path.abspath(directory / path)))


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

    def reachable_nodes(self):
        '''Visit the inputs of the requested targets, excluding order-only inputs.'''
        reachable = set()
        pending = list(self.targets)
        while pending:
            path = pending.pop()
            if path in reachable:
                continue
            reachable.add(path)
            if path in self.nodes:
                pending.extend(self.nodes[path].inputs)
        for path in sorted(reachable):
            node = self.nodes.get(path)
            if node is not None:
                yield path, node


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


def _archive_member(text: str, directory: Path) -> tuple[Artifact, str] | None:
    '''Split archive(member) from the rest of a linker-map line in linear time.'''
    lower = text.lower()
    starts = [lower.find(suffix) for suffix in ('.a(', '.lib(')]
    start = min((position for position in starts if position >= 0), default=-1)
    if start < 0:
        return None
    opening = text.index('(', start)
    member, closing, remainder = text[opening + 1 :].partition(')')
    if not closing or '(' in member:
        return None
    return Artifact(build_path(text[:opening], directory), member), remainder.lstrip()


def _link_consumer(text: str, directory: Path) -> Artifact | None:
    '''Read the referring object from "object (symbol)"; ignore entries without one.'''
    prefix, opening, symbol = text.rpartition('(')
    if not opening or not symbol.endswith(')'):
        return None
    symbol = symbol[:-1]
    if not symbol or ')' in symbol:
        return None
    if not prefix or not prefix[-1].isspace():
        return None
    consumer = prefix.rstrip()
    member = _archive_member(consumer, directory)
    if member:
        return member[0] if not member[1] else None
    if Path(consumer).suffix.lower() in {'.o', '.obj'}:
        return Artifact(build_path(consumer, directory))
    return None


def _archive_references(lines: list[str], directory: Path) -> set[tuple[Artifact, Artifact]]:
    '''Read archive inclusions, including records whose referring object wraps to the next line.'''
    result = set()
    provider = None
    for line in lines:
        if line and not line[0].isspace():
            member = _archive_member(line, directory)
            if member is None:
                break
            provider, remainder = member
        else:
            remainder = line.strip()
        if not remainder or provider is None:
            continue
        consumer = _link_consumer(remainder, directory)
        if consumer is not None:
            result.add((consumer, provider))
        provider = None
    return result


def read_link_references(map_file: Path, directory: Path) -> set[tuple[Artifact, Artifact]]:
    '''Read GNU ld's archive-inclusion table, not LOAD/section co-occurrence.

    The table is optional (e.g. lld and LTO). Whole-archive and command-line symbol
    entries have no referring object and therefore cannot establish a package edge.
    '''
    lines = map_file.read_text(errors='replace').splitlines()
    start = next(
        (
            i
            for i, line in enumerate(lines)
            if line.startswith('Archive member included to satisfy reference')
        ),
        None,
    )
    if start is None:
        return set()
    return _archive_references(lines[start + 1 :], directory)


def _install_sources(destination: str, arguments: str, prefix: str, directory: Path):
    '''Get source and destination paths from a literal CMake file installation.'''
    destination = destination.replace('${CMAKE_INSTALL_PREFIX}', prefix)
    if '$' in destination:
        return
    files = re.search(r'\bFILES\s+(.*)', arguments, re.S)
    if not files:
        return
    rename = re.search(r'\bRENAME "([^"]+)"', arguments)
    for source in re.findall(r'"([^"]+)"', files[1].split(' RENAME ')[0]):
        if '$' in source:
            continue
        source_path = build_path(source, directory)
        name = rename[1] if rename else source_path.name
        yield build_path(Path(destination) / name, directory), source_path


def _file_installs(script: Path, directory: Path):
    '''Read supported file installation commands without executing the script.'''
    text = script.read_text(errors='replace')
    prefix = re.search(r'set\(CMAKE_INSTALL_PREFIX "([^"]+)"\)', text)
    install_prefix = prefix[1] if prefix else '${CMAKE_INSTALL_PREFIX}'
    for match in re.finditer(
        r'file\(INSTALL DESTINATION "([^"]+)" TYPE (?:FILE|STATIC_LIBRARY|'
        r'SHARED_LIBRARY|EXECUTABLE) (.*?)\)',
        text,
        re.S,
    ):
        yield from _install_sources(match[1], match[2], install_prefix, directory)


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
        for target, source in _file_installs(script, directory):
            if target in installed:
                aliases.setdefault(target, set()).add(source)
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
            for path, node in graph.reachable_nodes():
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
            owners = self._archive_member_owner(artifact, node, file)
            if owners is not None:
                return owners
        return self._file_or_node_owner(path, node, file)

    def _archive_member_owner(self, artifact: Artifact, node: BuildNode, file) -> set[str] | None:
        # Archive-member basenames alone are insufficient when duplicated.
        members = [
            p for p in node.explicit if p.name == Path(artifact.member.replace('\\', '/')).name
        ]
        if len(members) == 1:
            return set(self.owner(Artifact(members[0])))
        if file is None or not file.package:
            return set()
        return None

    def _file_or_node_owner(self, path: Path, node: BuildNode | None, file) -> set[str]:
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
        self._record_unresolved_binaries()
        for target, node in self.nodes.items():
            self._collect_compile_inputs(target, node)
            self._collect_link_inputs(target, node)
        self._collect_archive_references()
        for consumer, provider in sorted(self.dependencies):
            package = self.data.packages[consumer]
            package.dependencies = sorted(set(package.dependencies) | {provider})
        self._report_unresolved()

    def _record_unresolved_binaries(self):
        for path in self.context.files:
            if path.suffix.lower() in BINARY_SUFFIXES:
                node = self.nodes.get(path)
                if not node or not node.inputs:
                    self.unresolved.add(path)

    def _collect_compile_inputs(self, target: Path, node: BuildNode):
        primary = {p for p in node.explicit if p.suffix.lower() in SOURCE_SUFFIXES}
        if not primary or target.suffix.lower() not in {'.o', '.obj'}:
            return
        consumers = set(self.owner(Artifact(target)))
        # Do not assert that every package in a mixed source rule consumes every input.
        if len(consumers) != 1:
            self.unresolved.add(target)
            return
        if not node.compiler:
            self.unresolved.add(target)
        for dependency in node.inputs - primary:
            self.add(consumers, self.providers(dependency))

    def _collect_link_inputs(self, target: Path, node: BuildNode):
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

    def _collect_archive_references(self):
        for consumer, provider in self.context.link_references:
            consumers = set(self.owner(consumer))
            providers = set(self.owner(provider))
            if len(consumers) == 1 and len(providers) == 1:
                self.add(consumers, providers)
            else:
                self.unresolved.add(provider.path)

    def _report_unresolved(self):
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
