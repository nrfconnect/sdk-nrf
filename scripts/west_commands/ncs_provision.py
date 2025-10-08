#!/usr/bin/env python3
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import argparse
import subprocess
import sys
import tempfile
import textwrap
from dataclasses import dataclass
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).parents[1]))
import generate_psa_key_attributes as psa_attr_generator
import yaml
from west.commands import WestCommand

KEY_SLOTS: dict[str, list[int]] = {
    "UROT_PUBKEY": [226, 228, 230],
    "BL_PUBKEY": [242, 244, 246],
    "APP_PUBKEY": [202, 204, 206],
}
POLICIES = ["revokable", "lock", "lock-last"]
NRF54L15_KEY_POLICIES: dict[str, str] = {
    "revokable": psa_attr_generator.PsaKeyPersistence.PERSISTENCE_REVOKABLE,
    "lock": psa_attr_generator.PsaKeyPersistence.PERSISTENCE_READ_ONLY,
}


@dataclass
class SlotParams:
    id: int
    keyfile: str
    lifetime: psa_attr_generator.PsaKeyPersistence


class NrfutilWrapper:
    def __init__(
        self,
        slots: list[SlotParams],
        device_id: str | None = None,
        output_dir: str | None = None,
        *,
        dry_run: bool = False,
    ) -> None:
        self.device_id = device_id
        self.slots = slots
        self.dry_run = dry_run
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
        """Generate PSA key attribute file, see generate_psa_key_attributes.py for details"""
        output_file = (
            Path(self.output_dir).joinpath("keyfile.json").resolve().expanduser()
        )

        for slot in self.slots:
            attr = psa_attr_generator.PlatformKeyAttributes(
                key_type=psa_attr_generator.PsaKeyType.ECC_PUBLIC_KEY_TWISTED_EDWARDS,
                identifier=slot.id,
                location=psa_attr_generator.PsaKeyLocation.LOCATION_CRACEN_KMU,
                persistence=slot.lifetime,
                key_usage=psa_attr_generator.PsaKeyUsage.VERIFY,
                algorithm=psa_attr_generator.PsaAlgorithm.EDDSA_PURE,
                key_bits=255,
                cracen_usage=psa_attr_generator.PsaCracenUsageScheme.RAW,
            )

            with open(slot.keyfile, "rb") as key_file:
                psa_attr_generator.generate_attr_file(
                    attributes=attr, key_file=output_file, key_from_file=key_file
                )
        return str(output_file)

    def _build_command(self) -> list[str]:
        json_file_path = self._make_json_file()
        command = [
            "nrfutil",
            "device",
            "x-provision-keys",
            "--key-file",
            json_file_path,
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
            formatter_class=argparse.RawDescriptionHelpFormatter,
        )
        group = upload_parser.add_mutually_exclusive_group(required=True)
        group.add_argument(
            "-i", "--input", metavar="PATH", help="Upload keys from YAML file"
        )
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
            "-s",
            "--soc",
            help="Deprecated, is not used anymore."
        )
        upload_parser.add_argument("--dev-id", help="Device serial number")
        upload_parser.add_argument(
            "--build-dir",
            metavar="PATH",
            help="Path to output directory where keyfile.json will be saved. "
            "If not specified, temporary directory will be used.",
        )
        upload_parser.add_argument(
            "--dry-run",
            default=False,
            action="store_true",
            help="Generate upload command and keyfile without executing the command",
        )

        return parser

    def do_run(self, args, unknown_args):
        if args.command == "upload":
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
            slots=slots,
            device_id=args.dev_id,
            output_dir=args.build_dir,
            dry_run=args.dry_run,
        )
        runner.run_command()

    @staticmethod
    def _read_keys_params_from_args(args: argparse.Namespace) -> list[dict[str, Any]]:
        data = [dict(keyname=args.keyname, policy=args.policy, keys=args.keys)]
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
            if policy == "lock-last":
                if slot_idx == (len(keys) - 1):
                    key_policy = NRF54L15_KEY_POLICIES["lock"]
                else:
                    key_policy = NRF54L15_KEY_POLICIES["revokable"]
            else:
                key_policy = NRF54L15_KEY_POLICIES[policy]
            slot_id = KEY_SLOTS[keyname][slot_idx]
            slot = SlotParams(id=slot_id, keyfile=keyfile, lifetime=key_policy)
            slots.append(slot)
        return slots

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
