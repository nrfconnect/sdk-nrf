# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''
The "ncs-sbom" extension command.
'''

import argparse
import os
import sys
from pathlib import Path

import args
from west.commands import WestCommand


class NcsSbom(WestCommand):
    '''"west ncs-sbom" command extension class.'''

    def __init__(self):
        super().__init__('ncs-sbom', args.COMMAND_DESCRIPTION, args.COMMAND_HELP)

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help,
            formatter_class=argparse.ArgumentDefaultsHelpFormatter,
            description=self.description)
        args.add_arguments(parser)
        return parser

    def do_run(self, arguments, unknown_arguments):
        zephyr_scripts = str(Path(os.environ['ZEPHYR_BASE']) / 'scripts')
        if zephyr_scripts not in sys.path:
            sys.path.insert(0, zephyr_scripts)

        # zephyr_module is available only after Zephyr's scripts directory is on sys.path.
        # Delay importing the SBOM pipeline until zephyr_scripts is available.
        import main

        args.copy_arguments(arguments)
        main.main()
