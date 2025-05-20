#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''
Parsing and utility functions for west ncs-sbom command arguments.
'''

import argparse
from pathlib import Path
from common import SbomException


DEFAULT_REPORT_NAME = 'sbom_report.html'

COMMAND_DESCRIPTION = 'Create license report for application'

COMMAND_HELP = '''
Create license report from source files.
'''

DETECTORS_HELP = '''
spdx-tag
  Search for the SPDX-License-Identifier in the source code or the binary file.
  For guidelines, see:
  https://spdx.github.io/spdx-spec/using-SPDX-short-identifiers-in-source-files

full-text
  Compare the contents of the license with the references that are stored in the database.

scancode-toolkit
  License detection by scancode-toolkit.
  For more details see: https://scancode-toolkit.readthedocs.io/en/stable/

cache-databese
  License detection is based on a predefined database.
  The license type is obtained from the database.

git-info
  Detects the source repository of each file and its commit hash.
'''


class ArgsClass:
    ''' Lists all command line arguments for better type hinting. '''
    _initialized: bool = False
    # arguments added by west
    help: 'bool|None'
    zephyr_base: 'str|None'
    verbose: int
    command: str
    # command arguments
    build_dir: 'list[list[str]]|None'
    input_files: 'list[list[str]]|None'
    input_list_file: 'list[str]|None'
    license_detectors: 'list[str]'
    optional_license_detectors: 'set[str]'
    output_html: 'str|None'
    output_cache_database: 'str|None'
    input_cache_database: 'str|None'
    allowed_in_map_file_only: 'str'
    processes: int
    scancode: str
    git: str
    ar: 'str|None'
    ninja: 'str|None'
    help_detectors: bool


def split_arg_list(text: str) -> 'list[str]':
    '''Split comma separated list removing whitespaces and empty items'''
    arr = text.split(',')
    arr = [x.strip() for x in arr]
    arr = list(filter(lambda x: len(x) > 0, arr))
    return arr


def split_detectors_list(allowed_detectors: dict, text: str) -> 'list[str]':
    '''Split comma separated list of detectors removing whitespaces, empty items and validating.'''
    arr = split_arg_list(text)
    for name in arr:
        if name not in allowed_detectors:
            raise SbomException(f'Detector not found: {name}')
    return arr


def add_arguments(parser: argparse.ArgumentParser):
    '''Add ncs-sbom specific arguments for parsing.'''
    parser.add_argument('-d', '--build-dir', nargs='+', action='append',
                        help='Build input directory. You can provide this option more than once.')
    parser.add_argument('--input-files', nargs='+', action='append',
                        help='Input files. You can use globs (?, *, **) to provide more files. '
                             'You can start argument with the exclamation mark to exclude file '
                             'that were already found starting from the last "--input-files". '
                             'You can provide this option more than once.')
    parser.add_argument('--input-list-file', action='append',
                        help='Reads list of files from a file. Works the same as "--input-files". '
                             'with arguments from each line of the file.'
                             'You can provide this option more than once.')
    parser.add_argument('--license-detectors',
                        default='spdx-tag,full-text,external-file,scancode-toolkit,git-info',
                        help='Comma separated list of enabled license detectors.')
    parser.add_argument('--optional-license-detectors', default='scancode-toolkit',
                        help='Comma separated list of optional license detectors. Optional license '
                             'detector is skipped if any of the previous detectors has already '
                             'detected any license.')
    parser.add_argument('--output-html', default=None,
                        help='Generate output HTML report.')
    parser.add_argument('--output-spdx', default=None,
                        help='Generate output SPDX report.')
    parser.add_argument('--output-cache-database', default=None,
                        help='Generate a license database for the files using scancode-toolkit')
    parser.add_argument('--input-cache-database', default=None,
                        help='Input license database. The database is passed to the "cache-databe" '
                             'detector')
    parser.add_argument('--allowed-in-map-file-only',
                        default='libgcc.a,'
                                'libc_nano.a,libc++_nano.a,libm_nano.a,'
                                'libc.a,libc++.a,libm.a',
                        help='Comma separated list of file names which can be detected in a map '
                             'file, but not visible in the build system. Usually, automatically '
                             'linked toolchain libraries or libraries linked by specifying custom '
                             'linker options.')
    parser.add_argument('-n', '--processes', type=int, default=0,
                        help='Scan using n parallel processes. By default, the number of processes '
                             'is equal to the number of processor cores.')
    parser.add_argument('--scancode', default='scancode',
                        help='Path to scancode-toolkit executable.')
    parser.add_argument('--git', default='git',
                        help='Path to git executable.')
    parser.add_argument('--ar', default=None,
                        help='Path to GNU binutils "ar" executable. '
                             'By default, it will be automatically detected.')
    parser.add_argument('--ninja', default=None,
                        help='Path to "ninja" executable. '
                             'By default, it will be automatically detected.')
    parser.add_argument('--help-detectors', action='store_true',
                        help='Show help for each available detector and exit.')


def copy_arguments(source):
    '''Copy arguments from source object to exported args variable.'''
    for name in source.__dict__:
        args.__dict__[name] = source.__dict__[name]
    args._initialized = True


def init_args(allowed_detectors: dict):
    '''Parse, validate and postprocess arguments'''

    if not args._initialized:
        # Parse command line arguments if running outside west
        parser = argparse.ArgumentParser(description=COMMAND_DESCRIPTION,
                                         formatter_class=argparse.ArgumentDefaultsHelpFormatter,
                                         allow_abbrev=False)
        add_arguments(parser)
        copy_arguments(parser.parse_args())

    if args.help_detectors:
        print(DETECTORS_HELP)
        exit()

    # Validate and postprocess arguments
    args.license_detectors = split_detectors_list(allowed_detectors, args.license_detectors)
    args.optional_license_detectors = set(split_detectors_list(allowed_detectors,
                                                               args.optional_license_detectors))
    args.allowed_in_map_file_only = set(split_arg_list(args.allowed_in_map_file_only))

    # Use default build directory if exists and there is no other input
    if (args.build_dir is None
            and (args.input_files is None or len(args.input_files) == 0)
            and (args.input_list_file is None or len(args.input_list_file) == 0)):
        from input_build import get_default_build_dir # Avoid circular import
        default_build_dir = get_default_build_dir()
        if default_build_dir is not None:
            args.build_dir = [[default_build_dir]]

    # By default, place HTML output in the build directory
    if (args.output_html is None) and (args.build_dir is not None):
        args.output_html = Path(args.build_dir[0][0]) / DEFAULT_REPORT_NAME


args: 'ArgsClass' = ArgsClass()
