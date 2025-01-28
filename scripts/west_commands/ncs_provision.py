#!/usr/bin/env python3
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

from cryptography.hazmat.primitives.serialization import load_pem_private_key
from west.commands import WestCommand

KEY_SLOTS: dict[str, list[int]] = {
    "UROT_PUBKEY": [226, 228, 230],
    "BL_PUBKEY": [242, 244, 246],
    "APP_PUBKEY": [202, 204, 206],
}
KEY_SLOT_METADATA: str = "0x10ba0030"
KMU_KEY_SLOT_DEST_ADDR: str = "0x20000000"
ALGORITHM: str = "ED25519"
NRF54L15_KEY_POLICIES: dict[str, str] = {"revokable": "REVOKED", "lock": "LOCKED"}


class NcsProvision(WestCommand):

    def __init__(self):
        super().__init__(
            "ncs-provision",
            "NCS provision",
            "NCS provision utility tool.",
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description)

        subparsers = parser.add_subparsers(dest="command")
        upload_parser = subparsers.add_parser("upload", help="Send to KMU")
        upload_parser.add_argument(
            "-k",
            "--key",
            type=Path,
            action="append",
            dest="keys",
            help="Input .pem file with ED25519 private key",
        )
        upload_parser.add_argument(
            "--keyname",
            choices=KEY_SLOTS.keys(),
            # default value for backward compatibility
            default="UROT_PUBKEY",
            help="Key name to upload",
        )
        upload_parser.add_argument(
            "-p",
            "--policy",
            type=str,
            choices=["revokable", "lock", "lock-last"],
            default="lock-last",
            help="Policy applied to the given set of keys. "
                 "revokable: keys can be revoked each by one. "
                 "lock: all keys stay as they are. "
                 "lock-last: last key is uploaded as locked, "
                 "others as revokable",
        )
        upload_parser.add_argument(
            "-s", "--soc", type=str, help="SoC",
            choices=["nrf54l05", "nrf54l10", "nrf54l15"], required=True
        )
        upload_parser.add_argument("--dev-id", help="Device serial number")

        return parser

    def do_run(self, args, unknown_args):
        if args.command == "upload":
            if args.soc in ["nrf54l05", "nrf54l10", "nrf54l15"]:
                keyname = args.keyname
                if len(args.keys) > len(KEY_SLOTS[keyname]):
                    sys.exit(
                        "Error: requested upload of more keys than there are designated slots.")
                for slot_idx, keyfile in enumerate(args.keys):
                    with open(keyfile, "rb") as f:
                        priv_key = load_pem_private_key(
                            f.read(), password=None)
                    pub_key = priv_key.public_key()
                    if args.policy == "lock-last":
                        if slot_idx == (len(args.keys) - 1):
                            key_policy = NRF54L15_KEY_POLICIES["lock"]
                        else:
                            key_policy = NRF54L15_KEY_POLICIES["revokable"]
                    else:
                        key_policy = NRF54L15_KEY_POLICIES[args.policy]
                    dev_id = args.dev_id
                    pub_key_hex = pub_key.public_bytes_raw().hex()
                    slot_id = str(KEY_SLOTS[keyname][slot_idx])
                    command = self._build_command(
                        dev_id=dev_id, key_policy=key_policy, pub_key=pub_key_hex, slot_id=slot_id
                    )
                    nrfprovision = subprocess.run(
                        command, stderr=subprocess.PIPE, text=True
                    )
                    stderr = nrfprovision.stderr
                    print(stderr, file=sys.stderr)
                    if re.search("fail", stderr) or nrfprovision.returncode:
                        sys.exit("Uploading failed!")

    @staticmethod
    def _build_command(
        key_policy: str, pub_key: str, slot_id: str, dev_id: str | None
    ) -> list[str]:
        command = [
            "nrfprovision",
            "provision",
            "--rpolicy",
            key_policy,
            "--value",
            pub_key,
            "--metadata",
            KEY_SLOT_METADATA,
            "--id",
            slot_id,
            "--algorithm",
            ALGORITHM,
            "--dest",
            KMU_KEY_SLOT_DEST_ADDR,
            "--verify",
        ]
        if dev_id:
            command.extend(["--snr", dev_id])
        return command
