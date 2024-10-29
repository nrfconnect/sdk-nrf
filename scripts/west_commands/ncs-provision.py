#!/usr/bin/env python3
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from pathlib import Path
import re
import sys
import subprocess
from cryptography.hazmat.primitives.serialization import load_pem_private_key
from west.commands import WestCommand

nrf54l15_key_slots = [226, 228, 230]


class NcsProvision(WestCommand):
    def __init__(self):
        super().__init__(
            "ncs-provision",
            "NCS provision",
            "NCS provision utility tool.",
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )

        subparsers = parser.add_subparsers(
            dest="command"
        )
        upload_parser = subparsers.add_parser("upload", help="Send to KMU")
        upload_parser.add_argument(
            "-k", "--key", type=Path, action='append', dest="keys",
            help="Input .pem file with ED25519 private key"
        )
        upload_parser.add_argument("-s", "--soc", type=str, help="SoC",
                                   choices=["nrf54l15"], required=True)
        upload_parser.add_argument("--dev-id", help="Device serial number")

        return parser

    def do_run(self, args, unknown_args):
        if args.command == "upload":
            if args.soc == "nrf54l15":
                if len(args.keys) > len(nrf54l15_key_slots):
                    sys.exit(
                        "Error: requested upload of more keys than there are designated slots.")
                slot = 0
                for keyfile in args.keys:
                    with open(keyfile, 'rb') as f:
                        priv_key = load_pem_private_key(f.read(), password=None)
                    pub_key = priv_key.public_key()
                    command = [
                        "nrfprovision",
                        "provision",
                        "-r",
                        "REVOKED",
                        "-v",
                        pub_key.public_bytes_raw().hex(),
                        "-m",
                        "0x10ba0030",
                        "-i",
                        str(nrf54l15_key_slots[slot]),
                        "-a",
                        "ED25519",
                        "-d",
                        "0x20000000",
                        "--verify"
                    ]
                    if args.dev_id:
                        command.extend(["--snr", args.dev_id])
                    nrfprovision = subprocess.run(
                        command,
                        stderr=subprocess.PIPE,
                        text=True
                    )
                    stderr = nrfprovision.stderr
                    print(stderr, file=sys.stderr)
                    if re.search('fail', stderr) or nrfprovision.returncode:
                        sys.exit("Uploading failed!")
                    slot += 1
