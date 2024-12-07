# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import sys
import argparse
from pathlib import Path


class ArgsClass:
    new_input: Path
    old_input: 'Path | None'
    format: str
    resolve_paths: 'Path | None'
    relative_to: 'Path | None'
    save_stats: 'Path | None'
    save_input: 'Path | None'
    save_old_input: 'Path | None'
    dump_json: 'Path | None'


def parse_args() -> ArgsClass:
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False,
                                     description='Detect API changes based on doxygen XML output.')
    parser.add_argument('new_input', metavar='new-input', type=Path,
                        help='The directory containing doxygen XML output or pre-parsed file with ' +
                             'the new API version. For details about ' +
                             'doxygen XML output, see https://www.doxygen.nl/manual/output.html.')
    parser.add_argument('old_input', metavar='old-input', nargs='?', type=Path,
                        help='The directory containing doxygen XML output or pre-parsed file with ' +
                             'the old API version. You should skip this if you want to pre-parse ' +
                             'the input with the "--save-input" option.')
    parser.add_argument('--format', choices=('text', 'github'), default='text',
                        help='Output format. Default is "text".')
    parser.add_argument('--resolve-paths', type=Path,
                        help='Resolve relative paths from doxygen input using this parameter as ' +
                             'base directory.')
    parser.add_argument('--relative-to', type=Path,
                        help='Show relative paths in messages.')
    parser.add_argument('--save-stats', type=Path,
                        help='Save statistics to JSON file.')
    parser.add_argument('--save-input', metavar='FILE', type=Path,
                        help='Pre-parse and save the "new-input" to a file. The file format may change ' +
                             'from version to version. Use always the same version ' +
                             'of this tool for one file.')
    parser.add_argument('--save-old-input', metavar='FILE', type=Path,
                        help='Pre-parse and save the "old-input" to a file.')
    parser.add_argument('--dump-json', metavar='FILE', type=Path,
                        help='Dump input data to a JSON file (only for debug purposes).')
    parser.add_argument('--help', action='help',
                        help='Show this help and exit.')
    args: ArgsClass = parser.parse_args()

    if (args.old_input is None) and (args.save_input is None):
        parser.print_usage()
        print('error: at least one of the following arguments is required: old-input, --save-input')
        sys.exit(2)

    args.resolve_paths = args.resolve_paths.absolute() if args.resolve_paths else None
    args.relative_to = args.relative_to.absolute() if args.relative_to else None

    return args


args: ArgsClass = parse_args()
