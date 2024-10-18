# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import sys
import argparse
from pathlib import Path


class ArgsClass:
    new: 'list[list[str]]'
    old: 'list[list[str]]|None'
    format: str
    relative_to: 'Path | None'
    save_stats: 'Path | None'
    save_input: 'Path | None'
    save_old_input: 'Path | None'
    dump_json: 'Path | None'


def parse_args() -> ArgsClass:
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False,
                                     description='Detect DTS binding changes.')
    parser.add_argument('-n', '--new', nargs='+', action='append', required=True,
                        help='List of directories where to search the new DTS binding. ' +
                             'The "-" will use the "ZEPHYR_BASE" environment variable to find ' +
                             'DTS binding in default directories.')
    parser.add_argument('-o', '--old', nargs='+', action='append',
                        help='List of directories where to search the old DTS binding. ' +
                             'The "-" will use the "ZEPHYR_BASE" environment variable to find ' +
                             'DTS binding in default directories. You should skip this if you ' +
                             'want to pre-parse the input with the "--save-input" option.')
    parser.add_argument('--format', choices=('text', 'github'), default='text',
                        help='Output format. Default is "text".')
    parser.add_argument('--relative-to', type=Path,
                        help='Show relative paths in messages.')
    parser.add_argument('--save-stats', type=Path,
                        help='Save statistics to JSON file.')
    parser.add_argument('--save-input', metavar='FILE', type=Path,
                        help='Pre-parse and save the new input to a file. The file format may change ' +
                             'from version to version. Use always the same version ' +
                             'of this tool for one file.')
    parser.add_argument('--save-old-input', metavar='FILE', type=Path,
                        help='Pre-parse and save the old input to a file.')
    parser.add_argument('--dump-json', metavar='FILE', type=Path,
                        help='Dump input data to a JSON file (only for debug purposes).')
    parser.add_argument('--help', action='help',
                        help='Show this help and exit.')
    args: ArgsClass = parser.parse_args()

    if (args.old is None) and (args.save_input is None):
        parser.print_usage()
        print('error: at least one of the following arguments is required: old-input, --save-input', file=sys.stderr)
        sys.exit(2)

    args.relative_to = args.relative_to.absolute() if args.relative_to else None

    return args


args: ArgsClass = parse_args()
