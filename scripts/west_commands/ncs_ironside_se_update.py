#!/usr/bin/env python3
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import argparse
import json
import struct
import subprocess
import time
from ctypes import c_char_p
from pathlib import Path, PosixPath
from tempfile import TemporaryDirectory
from zipfile import ZipFile

from west.commands import WestCommand

IRONSIDE_SE_VERSION_ADDR = 0x2F88FD04
IRONSIDE_SE_RECOVERY_VERSION_ADDR = 0x2F88FD14
UPDATE_STATUS_ADDR = 0x2F88FD24

UPDATE_STATUS_MSG = {
    0xFFFFFFFF: "None",
    0xF0000001: "UnknownOperation",
    0xF0000002: "InvalidManifest",
    0xF0000003: "StaleFW",
    0xF0000005: "VerifyFailure",
    0xF0000006: "VerifyOK",
    0xF0000007: "UROTUpdateDisabled",
    0xF0000008: "UROTActivated",
    0xF0000009: "RecoveryActivated",
    0xF000000A: "RecoveryUpdateDisabled",
    0xF100000A: "AROTRecovery",
}


class NcsIronSideSEUpdate(WestCommand):
    def __init__(self):
        super().__init__(
            name="ncs-ironside-se-update",
            help="NCS IronSide SE update",
            description="Update IronSide SE.",
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )

        parser.add_argument(
            "--zip",
            help="Path to IronSide SE release ZIP",
            type=argparse.FileType(mode="r"),
        )

        parser.add_argument(
            "--recovery",
            help="Update IronSide SE recovery instead of IronSide SE itself",
            action="store_true",
        )

        parser.add_argument(
            "--allow-erase", action="store_true", help="Allow erasing the device"
        )
        parser.add_argument("--serial", type=str, help="serial number", default=None)
        parser.add_argument(
            "--wait", help="Wait time (in seconds) between resets", default=2
        )

        return parser

    def _decode_version(self, version_bytes: bytes) -> str:
        # First word is semantic versioned 8 bit each
        seqnum, patch, minor, major = struct.unpack("bbbb", version_bytes[0:4])

        extraversion = c_char_p(version_bytes[4:]).value.decode("utf-8")

        return f"{major}.{minor}.{patch}-{extraversion}+{seqnum}"

    def _decode_status(self, status_bytes: bytes) -> str:
        status = int.from_bytes(status_bytes, "little")
        try:
            return f"{hex(status)} - {UPDATE_STATUS_MSG[status]}"
        except KeyError:
            return f"{hex(status)} - (unknown)"

    def do_run(self, args, unknown_args):
        def nrfutil_device(cmd: str) -> str:
            cmd = f"nrfutil device {cmd} --serial-number {args.serial}"
            self.dbg(cmd)

            result = subprocess.run(cmd, shell=True, text=True, capture_output=True)

            self.dbg(result.stdout)

            if result.returncode != 0:
                self.die(f"{cmd} returned '{result.returncode}' and '{result.stderr.strip()}'")

            return result.stdout

        def nrfutil_read(address: int, num_bytes: int) -> bytes:
            json_out = json.loads(
                nrfutil_device(
                    f"x-read --direct --json --skip-overhead --address 0x{address:x} --bytes {num_bytes}"
                )
            )
            return bytes(json_out["devices"][0]["memoryData"][0]["values"])

        def get_version() -> str:
            address = (
                IRONSIDE_SE_RECOVERY_VERSION_ADDR
                if args.recovery
                else IRONSIDE_SE_VERSION_ADDR
            )
            return self._decode_version(nrfutil_read(address, 16))

        def get_status() -> str:
            return self._decode_status(nrfutil_read(UPDATE_STATUS_ADDR, 4))

        def program(hex_file: PosixPath) -> None:
            if not hex_file.exists():
                self.die(f"Firmware file does not exist: {hex_file}")

            nrfutil_device(
                f"program --options chip_erase_mode=ERASE_NONE --firmware {hex_file}"
            )

        if not args.allow_erase:
            self.die(
                "Unable to perform update without erasing the device, set '--allow-erase'"
            )
        with TemporaryDirectory() as tmpdir:
            with ZipFile(args.zip.name, "r") as zip_ref:
                zip_ref.extractall(tmpdir)
                update_app = Path(tmpdir, "update/update_application.hex")
                if args.recovery:
                    update_hex = "ironside_se_recovery_update.hex"
                else:
                    update_hex = "ironside_se_update.hex"

                update_to_install = Path(tmpdir, "update", update_hex)

                # Check if required files exist in the extracted ZIP
                if not update_app.exists():
                    self.die(f"Update application file not found in ZIP: {update_app}")

                if not update_to_install.exists():
                    self.die(f"Update firmware file not found in ZIP: {update_to_install}")

                self.inf(
                    f"Version before update: {get_version()}, status: {get_status()}"
                )

                program(update_app)
                program(update_to_install)

                self.dbg("Reset to execute update service")
                nrfutil_device(f"reset --reset-kind RESET_PIN")
                time.sleep(args.wait)

                self.dbg("Reset to trigger update installation")
                nrfutil_device(f"reset --reset-kind RESET_PIN")
                self.dbg("Waiting for update to complete")
                time.sleep(args.wait)

                self.inf(
                    f"Version after update: {get_version()}, status: {get_status()}"
                )
                self.inf(
                    "The update application is still programmed, please program proper image"
                )
