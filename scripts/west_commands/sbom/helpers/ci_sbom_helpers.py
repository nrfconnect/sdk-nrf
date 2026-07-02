#!/usr/bin/env python3
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Helpers for the full-repository SBOM workflow.
'''

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path


def _run(command: tuple[str, ...], cwd: Path | None = None, text: bool = True) -> str | bytes:
    '''Run a command and return its stdout, raising on a non-zero exit.'''
    return subprocess.run(
        command,
        cwd=cwd,
        check=True,
        capture_output=True,
        text=text,
    ).stdout


def repository_root() -> Path:
    '''Return the absolute path of the current git repository root.'''
    return Path(_run(('git', 'rev-parse', '--show-toplevel')).strip()).resolve()


def tracked_files(root: Path) -> list[str]:
    '''Return sorted, on-disk tracked files of one git repo, relative to it.'''
    stdout = _run(
        ('git', '-c', 'core.quotepath=off', 'ls-files', '-z'),
        cwd=root,
        text=False,
    )
    files = []
    for raw_path in stdout.split(b'\0'):
        if not raw_path:
            continue
        rel_path = raw_path.decode('utf-8', errors='surrogateescape')
        # Skip gitlinks (submodules) and stale entries; keep only real files.
        if (root / rel_path).is_file():
            files.append(Path(rel_path).as_posix())
    files.sort()
    return files


def workspace_files(workspace: Path) -> list[str]:
    '''Return sorted tracked files across every west project in a workspace.'''
    stdout = _run(('west', 'list', '-f', '{path}'), cwd=workspace)
    files = []
    for line in stdout.splitlines():
        project_path = line.strip()
        if not project_path:
            continue
        for rel_path in tracked_files(workspace / project_path):
            files.append(Path(project_path, rel_path).as_posix())
    files.sort()
    return files


def shard_files(files: list[str], shards: int, shard_index: int) -> list[str]:
    '''Return one round-robin shard of the file list.'''
    return files[shard_index::shards]


def write_list_file(output: Path, files: list[str]) -> None:
    '''Write one path per line.'''
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(''.join(f'{path}\n' for path in files), encoding='utf-8')


def write_json_file(output: Path, data: dict) -> None:
    '''Write pretty-printed, key-sorted JSON with a trailing newline.'''
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open('w', encoding='utf-8') as fd:
        json.dump(data, fd, indent=2, sort_keys=True)
        fd.write('\n')


def matrix_command(args: argparse.Namespace) -> int:
    '''Print the GitHub Actions shard matrix as JSON on stdout.'''
    matrix = {'include': [{'shard_index': i} for i in range(args.shards)]}
    print(json.dumps(matrix, separators=(',', ':')))
    return 0


def list_files_command(args: argparse.Namespace) -> int:
    '''Write the full file list, or a single shard of it, to a file.'''
    if args.workspace is None:
        files = tracked_files(repository_root())
    else:
        files = workspace_files(Path(args.workspace).resolve())
    if args.shards is not None:
        files = shard_files(files, args.shards, args.shard_index)
    write_list_file(Path(args.output), files)
    print(f'Wrote {len(files)} file(s) to {args.output}', file=sys.stderr)
    return 0


def merge_cache_command(args: argparse.Namespace) -> int:
    '''Union the `files` maps of several cache databases into one.'''
    merged = {'files': {}}
    for input_path in args.inputs:
        with open(input_path, encoding='utf-8') as fd:
            merged['files'].update(json.load(fd).get('files', {}))
    write_json_file(Path(args.output), merged)
    return 0


def build_parser() -> argparse.ArgumentParser:
    '''Create the command-line parser.'''
    parser = argparse.ArgumentParser(
        description='Helpers for the full-repository SBOM workflow.',
        allow_abbrev=False,
    )
    subparsers = parser.add_subparsers(dest='command', required=True)

    matrix_parser = subparsers.add_parser('matrix', allow_abbrev=False)
    matrix_parser.add_argument('--shards', type=int, required=True)
    matrix_parser.set_defaults(func=matrix_command)

    list_files_parser = subparsers.add_parser('list-files', allow_abbrev=False)
    list_files_parser.add_argument('--output', required=True)
    list_files_parser.add_argument('--workspace')
    list_files_parser.add_argument('--shards', type=int)
    list_files_parser.add_argument('--shard-index', type=int, default=0)
    list_files_parser.set_defaults(func=list_files_command)

    merge_cache_parser = subparsers.add_parser('merge-cache', allow_abbrev=False)
    merge_cache_parser.add_argument('--output', required=True)
    merge_cache_parser.add_argument('inputs', nargs='+')
    merge_cache_parser.set_defaults(func=merge_cache_command)

    return parser


def main() -> int:
    '''Program entry point.'''
    args = build_parser().parse_args()
    return args.func(args)


if __name__ == '__main__':
    raise SystemExit(main())
