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
from enum import Enum
from pathlib import Path, PosixPath
from tempfile import TemporaryDirectory
from textwrap import indent
from typing import Any
from zipfile import ZipFile

from intelhex import IntelHex
from west.commands import WestCommand

IRONSIDE_SE_VERSION_ADDR = 0x2F88_FD04
IRONSIDE_SE_RECOVERY_VERSION_ADDR = 0x2F88_FD14

MANIFEST_VERSION_OFFSET = 0x60
IRONSIDE_VERSION_LEN = 16

UPDATE_STATUS_ADDR = 0x2F88_FD24


class UpdateStatus(int, Enum):
    NONE = 0xFFFF_FFFF
    INVALID_MANIFEST = 0xF000_0002
    STALE_FW = 0xF000_0003
    VERIFY_FAILURE = 0xF000_0005
    UROT_UPDATE_DISABLED = 0xF000_0007
    UROT_ACTIVATED = 0xF000_0008
    RECOVERY_ACTIVATED = 0xF000_0009
    RECOVERY_UPDATE_DISABLED = 0xF000_000A

    @property
    def description(self) -> str:
        match self:
            case UpdateStatus.NONE:
                return "no update status"
            case UpdateStatus.INVALID_MANIFEST:
                return "the manifest in the update blob contains invalid data"
            case UpdateStatus.STALE_FW:
                return (
                    "the firmware is older than the installed firmware, "
                    "and firmware downgrades are disabled on the device"
                )
            case UpdateStatus.VERIFY_FAILURE:
                return (
                    "failed to verify the signature of the firmware, possibly "
                    "due to it being signed using different keys than those installed in the device"
                )
            case UpdateStatus.UROT_UPDATE_DISABLED:
                return "updates of the IronSide SE firmware are disabled on the device"
            case UpdateStatus.UROT_ACTIVATED:
                return "the IronSide SE firmware was successfully updated"
            case UpdateStatus.RECOVERY_ACTIVATED:
                return "the IronSide SE Recovery firmware was successfully updated"
            case UpdateStatus.RECOVERY_UPDATE_DISABLED:
                return "updates of the IronSide SE Recovery firmware are disabled on the device"
            case _:
                return "unrecognized update status"


class FirmwareSlot(str, Enum):
    USLOT = "uslot"
    RSLOT = "rslot"

    @property
    def description(self) -> str:
        match self:
            case FirmwareSlot.USLOT:
                return "IronSide SE"
            case FirmwareSlot.RSLOT:
                return "IronSide SE Recovery"


class NcsIronSideSEUpdate(WestCommand):
    def __init__(self) -> None:
        super().__init__(
            name="ncs-ironside-se-update",
            help="NCS IronSide SE update",
            description="Update IronSide SE.",
        )

    def do_add_parser(self, parser_adder: Any) -> argparse.ArgumentParser:
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )
        parser.add_argument(
            "--zip",
            required=True,
            help="Path to IronSide SE release ZIP",
            type=argparse.FileType(mode="r"),
        )
        parser.add_argument(
            "--serial",
            dest="serial_number",
            type=str,
            help="Serial number",
        )
        parser.add_argument(
            "--firmware-slot",
            help="Only update the given firmware slot instead of updating both slots",
            choices=[m.value for m in FirmwareSlot.__members__.values()],
            type=FirmwareSlot,
        )
        parser.add_argument(
            "--allow-erase",
            action="store_true",
            help="Allow erasing the device (this option is currently required)",
        )
        parser.add_argument(
            "--wait-time",
            type=float,
            help="Timeout in seconds to wait for the device to boot",
            default=2.0,
        )

        return parser

    def do_run(self, args: argparse.Namespace, unknown: list[str]) -> None:
        has_x_sdfw_variant = self._nrfutil_supports_sdfw_variant()
        common_kwargs = {
            "serial_number": args.serial_number,
            "wait_time": args.wait_time,
            "sdfw_variant": "ironside" if has_x_sdfw_variant else None,
        }

        if not args.allow_erase:
            self.die(
                "Unable to perform update without erasing the device, set '--allow-erase'"
            )

        with TemporaryDirectory() as tmpdir, ZipFile(args.zip.name, "r") as zip_ref:
            zip_ref.extractall(tmpdir)
            update_dir = Path(tmpdir, "update")
            update_app = update_dir / "update_application.hex"
            match args.firmware_slot:
                case FirmwareSlot.USLOT:
                    update_hex_files = [
                        (FirmwareSlot.USLOT, update_dir / "ironside_se_update.hex")
                    ]
                case FirmwareSlot.RSLOT:
                    update_hex_files = [
                        (
                            FirmwareSlot.RSLOT,
                            update_dir / "ironside_se_recovery_update.hex",
                        )
                    ]
                case _:
                    update_hex_files = [
                        (FirmwareSlot.USLOT, update_dir / "ironside_se_update.hex"),
                        (
                            FirmwareSlot.RSLOT,
                            update_dir / "ironside_se_recovery_update.hex",
                        ),
                    ]

            if not update_app.exists():
                self.die(
                    f"Update application file not found in ZIP: "
                    f"{update_app.relative_to(update_dir)}"
                )

            target_versions = {}
            for slot, update_file in update_hex_files:
                if not update_file.exists():
                    self.die(
                        f"Update firmware file for {slot.description} ({slot.value}) "
                        f"not found in ZIP: {update_file.relative_to(update_dir)}"
                    )
                target_versions[slot] = self._load_update_version(update_file)

            fw_version_before = self._read_versions_from_device(**common_kwargs)
            target_version_diff = self._fmt_versions(
                before=fw_version_before,
                after=target_versions,
            )
            self.inf(
                f"Updating IronSide SE firmware:\n{indent(target_version_diff, '  ')}\n"
            )

            self.inf("Erasing non-volatile memory")
            self._nrfutil_device("recover", **common_kwargs)

            self.dbg("Programming application firmware used to trigger the update")
            self._program(update_app, erase=True, **common_kwargs)

            update_failed = False
            last_update_status = UpdateStatus.NONE

            for slot, update_file in update_hex_files:
                slot_target_version = target_versions[slot]
                self.inf(
                    f"Updating {slot.description} ({slot.name}) "
                    f"to {slot_target_version}"
                )

                update_status = self._read_update_status(**common_kwargs)
                self.dbg(f"Status before triggering the update: {update_status}")

                self._program(update_file, **common_kwargs)

                self.dbg("Reset to execute update service")
                self._nrfutil_device("reset", **common_kwargs)
                # Wait for the application to boot
                self._wait_for_bootstatus(**common_kwargs)
                # Wait for the application to execute
                time.sleep(0.200)

                self.dbg("Reset to trigger update installation")
                self._nrfutil_device(
                    "reset --reset-kind RESET_VIA_SECDOM",
                    die_on_error=False, # nrfutil will incorrectly fail, so don't die on error
                    **common_kwargs,
                )
                self.dbg("Waiting for update to complete")
                self._wait_for_bootstatus(**common_kwargs)

                slot_version = self._read_firmware_version(slot, **common_kwargs)
                last_update_status = self._read_update_status(**common_kwargs)
                self.dbg(
                    f"Version after update: {slot_version}, status: {last_update_status.name}"
                )

                if slot_version != slot_target_version:
                    update_failed = True
                    self.err(
                        f"Failed to update {slot.description} ({slot.name}) "
                        f"to {slot_target_version}"
                    )
                    break

            self.dbg("Erasing application firmware used to trigger the update")
            self._nrfutil_device("erase --all", **common_kwargs)

            if update_failed:
                fw_version_after = self._read_versions_from_device(**common_kwargs)
                version_str = self._fmt_versions(
                    before=None, after=fw_version_after
                )
                self.inf(
                    f"The device is left with IronSide SE firmware:\n"
                    f"{indent(version_str, '  ')}\n"
                )
                if last_update_status != UpdateStatus.NONE:
                    fail_status_str = (
                        f"{last_update_status.name} - "
                        f"{last_update_status.description}"
                    )
                else:
                    fail_status_str = (
                        "The update status did not "
                        "indicate why the failure happened"
                    )
                self.die(fail_status_str)

    def _program(self, hex_file: PosixPath, erase: bool = False, **kwargs: Any) -> None:
        if not hex_file.exists():
            self.die(f"Firmware file does not exist: {hex_file}")

        options = " --options chip_erase_mode=ERASE_NONE" if not erase else ""
        self._nrfutil_device(f"program{options} --firmware {hex_file}", **kwargs)

    def _read_versions_from_device(self, **kwargs: Any) -> dict:
        return {
            FirmwareSlot.USLOT: self._read_firmware_version(
                FirmwareSlot.USLOT, **kwargs
            ),
            FirmwareSlot.RSLOT: self._read_firmware_version(
                FirmwareSlot.RSLOT, **kwargs
            ),
        }

    def _nrfutil_device(
        self,
        cmd: str,
        serial_number: str | None = None,
        sdfw_variant: str | None = None,
        die_on_error: bool = True,
        dbg_log_stdout: bool = True,
        **kwargs: Any,
    ) -> str:
        optional_args = ""
        if serial_number is not None:
            optional_args += f" --serial-number {serial_number}"
        if sdfw_variant is not None:
            optional_args += f"  --x-sdfw-variant {sdfw_variant}"

        cmd = f"nrfutil device {cmd}{optional_args}"
        self.dbg(cmd)

        result = subprocess.run(cmd, shell=True, text=True, capture_output=True)

        if dbg_log_stdout:
            self.dbg(result.stdout)

        if result.returncode != 0:
            if die_on_error:
                self.die(
                    f"{cmd} returned '{result.returncode}' and '{result.stderr.strip()}'"
                )
            else:
                return ""

        return result.stdout

    def _nrfutil_read(self, address: int, num_bytes: int, **kwargs: Any) -> bytes:
        json_out = json.loads(
            self._nrfutil_device(
                f"x-read --direct --address 0x{address:x} --bytes {num_bytes} "
                "--json --skip-overhead",
                **kwargs,
            )
        )
        return bytes(json_out["devices"][0]["memoryData"][0]["values"])

    def _nrfutil_supports_sdfw_variant(self) -> bool:
        nrfutil_device_program_helptext = self._nrfutil_device(
            "program --help",
            dbg_log_stdout=False,
        )
        return "--x-sdfw-variant" in nrfutil_device_program_helptext

    def _wait_for_bootstatus(self, wait_time: float, **kwargs: Any) -> int:
        boot_status = None
        start = time.perf_counter()
        while not boot_status:
            output_raw = self._nrfutil_device(
                "x-boot-status-get --json --skip-overhead",
                die_on_error=False,
                **kwargs,
            )
            if not output_raw:
                continue

            output_json = json.loads(output_raw)
            boot_status = output_json["devices"][0]["boot_status"]
            if (time.perf_counter() - start) >= wait_time:
                break

        if not boot_status:
            self.die("Timed out waiting for a non-zero bootstatus")

        return boot_status

    def _read_firmware_version(self, slot: FirmwareSlot, **kwargs: Any) -> str:
        address = (
            IRONSIDE_SE_RECOVERY_VERSION_ADDR
            if slot == FirmwareSlot.RSLOT
            else IRONSIDE_SE_VERSION_ADDR
        )
        return self._decode_version(self._nrfutil_read(address, 16, **kwargs))

    def _read_update_status(self, **kwargs: Any) -> UpdateStatus:
        return self._decode_status(self._nrfutil_read(UPDATE_STATUS_ADDR, 4, **kwargs))

    def _load_update_version(self, update_file: Path) -> str:
        self.dbg(f"Reading firmware version from {update_file}")
        ihex = IntelHex(str(update_file))
        version_addr = ihex.minaddr() + MANIFEST_VERSION_OFFSET
        version_bytes = bytes(
            [ihex[a] for a in range(version_addr, version_addr + IRONSIDE_VERSION_LEN)]
        )
        return self._decode_version(version_bytes)

    def _decode_version(self, version_bytes: bytes) -> str:
        # First word is semantic versioned 8 bit each
        seqnum, patch, minor, major = struct.unpack("bbbb", version_bytes[0:4])
        extraversion = c_char_p(version_bytes[4:]).value.decode("utf-8")
        return f"{major}.{minor}.{patch}-{extraversion}+{seqnum}"

    def _decode_status(self, status_bytes: bytes) -> UpdateStatus:
        status = int.from_bytes(status_bytes, "little")
        try:
            return UpdateStatus(status)
        except ValueError:
            self.die(
                f"Read unrecognized update status from the device: 0x{status:09_X}"
            )

    def _fmt_versions(
        self, *, before: dict[FirmwareSlot, str] | None, after: dict[FirmwareSlot, str]
    ) -> str:
        lines = []
        for slot in after:
            if before is not None:
                slot_versions = f"{before[slot]} -> {after[slot]}"
            else:
                slot_versions = after[slot]
            slot_name = f"{slot.description} ({slot.name})"
            lines.append(f"{slot_versions} - {slot_name}")
        return "\n".join(lines)
