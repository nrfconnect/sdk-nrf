#!/usr/bin/env python3
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
from pathlib import Path

import pem
from west.commands import WestCommand


class provision54l(WestCommand):
    def __init__(self):
        super(provision54l, self).__init__(
            "provision54l",
            "provision54l",
            "provision54l utility tool.",
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )

        subparsers = parser.add_subparsers(
            title="init",
            description="init subcommands",
            dest="command",
        )
        key_parser = subparsers.add_parser("upload", help="Send to KMU")
        key_parser.add_argument(
            "-k", "--key", type=Path, action='append', help="input pem", nargs='+'

        )

        return parser

    def do_run(self, args, unknown_args):
        if args.command == "upload":
            for keyfile in args.key:
                keyfile_path = Path(keyfile[0])
                print(keyfile_path)
                key = pem.parse_file(keyfile_path)
                priv_key = key[0].bytes_payload
                print(priv_key)
                #nrfprovision provision -r REVOKED -v  -m 0xedd25519 -i 6 -a ED25519 -d 0x20000000 --verify





