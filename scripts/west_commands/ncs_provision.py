#!/usr/bin/env python3
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import argparse
import json
import subprocess
import sys
import tempfile
import textwrap
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any

import yaml
from cryptography.hazmat.primitives.serialization import (
    load_pem_private_key,
    load_pem_public_key,
)
from west.commands import WestCommand

KEY_SLOTS: dict[str, list[int]] = {
    "UROT_PUBKEY": [226, 228, 230],
    "BL_PUBKEY": [242, 244, 246],
    "APP_PUBKEY": [202, 204, 206],
}
KMU_KEY_SLOT_DEST_ADDR: str = "0x20000000"
ALGORITHM: str = "ED25519"
POLICIES = ["revokable", "lock", "lock-last"]
NRF54L15_KEY_POLICIES: dict[str, str] = {"revokable": "REVOKED", "lock": "LOCKED"}

METADATA_ALGORITHM_MAPPING: dict[str, int] = {
    "ED25519": 10,
}
METADATA_RPOLICY_MAPPING: dict[str, int] = {
    'ROTATING': 1,
    'LOCKED': 2,
    'REVOKED': 3
}


# Implemented based on
# https://github.com/nrfconnect/sdk-nrf/blob/f3ac9cfb784d163f4c1af11209976fcd5c403d5a/subsys/nrf_security/src/drivers/cracen/cracenpsa/src/kmu.c#L41
@dataclass
class KmuMetadata:
    metadata_version: int = 0
    key_usage_scheme: int = 0
    reserved: int = 0
    algorithm: int = 0
    size: int = 0
    rpolicy: int = 0
    usage_flags: int = 0

    @property
    def value(self) -> int:
        # Combine all fields into a single 32-bit integer
        value = 0
        value |= (self.metadata_version & 0xF) << 0  # (4 bits)
        value |= (self.key_usage_scheme & 0x3) << 4  # (2 bits)
        value |= (self.reserved & 0x3FF) << 6  # (10 bits)
        value |= (self.algorithm & 0xF) << 16  # (4 bits)
        value |= (self.size & 0x7) << 20  # (3 bits)
        value |= (self.rpolicy & 0x3) << 23  # (2 bits)
        value |= (self.usage_flags & 0x7F) << 25  # (7 bits)

        return value

    @classmethod
    def from_value(cls, value: int):
        metadata_version = (value >> 0) & 0xF
        key_usage_scheme = (value >> 4) & 0x3
        reserved = (value >> 6) & 0x3FF
        algorithm = (value >> 16) & 0xF
        size = (value >> 20) & 0x7
        rpolicy = (value >> 23) & 0x3
        usage_flags = (value >> 25) & 0x7F

        return cls(
            metadata_version, key_usage_scheme, reserved,
            algorithm, size, rpolicy, usage_flags
        )

    def __str__(self) -> str:
        return f'0x{self.value:08x}'


@dataclass
class SlotParams:
    id: int
    value: str
    rpolicy: str
    metadata: str
    algorithm: str = ALGORITHM
    dest: str = KMU_KEY_SLOT_DEST_ADDR

    def asdict(self) -> dict[str, str]:
        return asdict(self)


class NrfutilWrapper:

    def __init__(
        self,
        slots: list[SlotParams],
        device_id: str | None = None,
        output_dir: str | None = None,
        *,
        dry_run: bool = False
    ) -> None:
        self.device_id = device_id
        self.dry_run = dry_run
        self.data = {
            "version": 0,
            "keyslots": [slot.asdict() for slot in slots]
        }
        self.output_dir = output_dir or tempfile.mkdtemp(prefix="nrfutil_")

    def run_command(self):
        command = self._build_command()
        print(" ".join(command), file=sys.stderr)
        if self.dry_run:
            return
        result = subprocess.run(command, stderr=subprocess.PIPE, text=True)
        if result.returncode:
            print(result.stderr, file=sys.stderr)
            sys.exit("Uploading failed!")
        else:
            print("Uploaded!", file=sys.stderr)

    def _make_json_file(self) -> str:
        """Create JSON file and return path to it."""
        json_file = Path(self.output_dir).joinpath("keyfile.json").resolve().expanduser()
        with open(json_file, "w") as file:
            json.dump(self.data, file, indent=2)
        print(f"Keys file saved as {json_file}", file=sys.stderr)
        return str(json_file)

    def _build_command(self) -> list[str]:
        json_file_path = self._make_json_file()
        command = [
            "nrfutil",
            "device",
            "x-provision-nrf54l-keys",
            "--key-file",
            json_file_path,
            "--verify",
        ]
        if self.device_id:
            command += ["--serial-number", self.device_id]
        else:
            command += ["--traits", "jlink"]

        return command


class NcsProvision(WestCommand):

    def __init__(self):
        super().__init__(
            name="ncs-provision",
            help="NCS provision",
            description="NCS provision utility tool.",
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )

        subparsers = parser.add_subparsers(dest="command")
        upload_parser = subparsers.add_parser(
            "upload",
            help="Send to KMU",
            epilog=textwrap.dedent("""
                Example input YAML file:
                    - keyname: UROT_PUBKEY
                      keys: ["key1.pem", "key2.pem"]
                      policy: lock
            """),
            formatter_class=argparse.RawDescriptionHelpFormatter
        )
        group = upload_parser.add_mutually_exclusive_group(required=True)
        group.add_argument("-i", "--input", metavar="PATH", help="Upload keys from YAML file")
        group.add_argument(
            "-k",
            "--key",
            type=Path,
            action="append",
            dest="keys",
            help="Input .pem file with ED25519 private or public key",
        )
        upload_parser.add_argument(
            "--keyname",
            choices=KEY_SLOTS.keys(),
            # default value for backward compatibility
            default="UROT_PUBKEY",
            type=lambda x: x.upper(),
            help="Key name to upload (default: %(default)s)",
        )
        upload_parser.add_argument(
            "-p",
            "--policy",
            type=str,
            choices=POLICIES,
            default="lock-last",
            help="Policy applied to the given set of keys. "
                 "revokable: keys can be revoked each by one. "
                 "lock: all keys stay as they are. "
                 "lock-last: last key is uploaded as locked, "
                 "others as revokable (default=%(default)s)",
        )
        upload_parser.add_argument(
            "-s", "--soc", type=str, help="SoC",
            choices=["nrf54l05", "nrf54l10", "nrf54l15"], required=True
        )
        upload_parser.add_argument("--dev-id", help="Device serial number")
        upload_parser.add_argument(
            "--build-dir", metavar="PATH",
            help="Path to output directory where keyfile.json will be saved. "
                 "If not specified, temporary directory will be used.",
        )
        upload_parser.add_argument(
            "--dry-run", default=False, action="store_true",
            help="Generate upload command and keyfile without executing the command"
        )

        return parser

    def do_run(self, args, unknown_args):
        if args.command == "upload":
            if args.soc in ["nrf54l05", "nrf54l10", "nrf54l15"]:
                self._upload_keys(args)

    def _upload_keys(self, args: argparse.Namespace) -> None:
        slots: list[SlotParams] = []
        if args.input:
            data = self._read_keys_params_from_file(args.input)
        else:
            data = self._read_keys_params_from_args(args)

        for value in data:
            slots += self._generate_slots(**value)

        runner = NrfutilWrapper(
            slots=slots, device_id=args.dev_id, output_dir=args.build_dir, dry_run=args.dry_run
        )
        runner.run_command()

    @staticmethod
    def _read_keys_params_from_args(args: argparse.Namespace) -> list[dict[str, Any]]:
        data = [
            dict(
                keyname=args.keyname,
                policy=args.policy,
                keys=args.keys
            )
        ]
        return data

    def _read_keys_params_from_file(self, filename: str) -> list[dict[str, Any]]:
        with open(filename) as file:
            try:
                data = yaml.safe_load(file)
            except yaml.YAMLError:
                sys.exit("Invalid YAML file")
            if not self._validate_input_file(data):
                sys.exit("Invalid input file format")
        return data

    def _generate_slots(self, keyname: str, keys: str, policy: str) -> list[SlotParams]:
        """Return list of SlotParams for given keys."""
        if len(keys) > len(KEY_SLOTS[keyname]):
            sys.exit(
                "Error: requested upload of more keys than there are designated slots."
            )
        slots: list[SlotParams] = []
        for slot_idx, keyfile in enumerate(keys):
            pub_key_hex = self._get_public_key_hex(keyfile)
            if policy == "lock-last":
                if slot_idx == (len(keys) - 1):
                    key_policy = NRF54L15_KEY_POLICIES["lock"]
                else:
                    key_policy = NRF54L15_KEY_POLICIES["revokable"]
            else:
                key_policy = NRF54L15_KEY_POLICIES[policy]
            slot_id = KEY_SLOTS[keyname][slot_idx]

            metadata = KmuMetadata(
                metadata_version=0,
                key_usage_scheme=3,
                reserved=0,
                algorithm=METADATA_ALGORITHM_MAPPING[ALGORITHM],
                size=3,
                rpolicy=METADATA_RPOLICY_MAPPING[key_policy],
                usage_flags=8
            )
            slot = SlotParams(
                id=slot_id, value=pub_key_hex, rpolicy=key_policy, metadata=str(metadata)
            )

            slots.append(slot)

        return slots

    @staticmethod
    def _get_public_key_hex(keyfile: str | Path) -> str:
        """Return the public key hex from the given keyfile."""
        with open(keyfile, "rb") as f:
            data = f.read()
            try:
                public_key = load_pem_public_key(data)
            except ValueError:
                # it seems it is not public key, so lets try with private
                private_key = load_pem_private_key(data, password=None)
                public_key = private_key.public_key()
        pub_key_hex = f"0x{public_key.public_bytes_raw().hex()}"
        return pub_key_hex

    @staticmethod
    def _validate_input_file(data: list[dict[str, str]]) -> bool:
        # simply yaml validator which does not require external package
        if not isinstance(data, list):
            return False
        for item in data:
            if not isinstance(item, dict):
                return False
            if {"keyname", "keys", "policy"} != set(item.keys()):
                return False
            if item["keyname"] not in KEY_SLOTS:
                return False
            if item["policy"] not in POLICIES:
                return False
            if not isinstance(item["keys"], list):
                return False
            for key in item["keys"]:
                if not isinstance(key, str):
                    return False
        return True
