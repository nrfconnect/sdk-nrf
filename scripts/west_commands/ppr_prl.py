#!/usr/bin/env python3
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import argparse
import json
import struct
import subprocess
import sys
import tempfile
import textwrap
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any

import yaml
from west.commands import WestCommand

nrf_path = "/"

header_size = 32
entry_size = 12


class NrfutilWrapperFlash:
    def __init__(
        self,
        image_path: str,
        device_id: str | None = None,
    ) -> None:
        self.device_id = device_id
        self.image_path = image_path

    def run_command(self):
        command = self._build_command()
        print(" ".join(command), file=sys.stderr)
        result = subprocess.run(command, stderr=subprocess.PIPE, text=True)
        if result.returncode:
            print(result.stderr, file=sys.stderr)
            sys.exit("Flashing failed!")
        else:
            print("Flashed!", file=sys.stderr)


    def _build_command(self) -> list[str]:
        command = [
            "nrfutil",
            "device",
            "program",
            "--options",
            "chip_erase_mode=ERASE_RANGES_TOUCHED_BY_FIRMWARE",
            "--core",
            "Application",
        ]
        command += ["--firmware", self.image_path]

        if self.device_id:
            command += ["--serial-number", self.device_id]
        else:
            command += ["--traits", "jlink"]

        return command


class NrfutilWrapperReadout:
    def __init__(
        self,
        address: str,
        entries_count: int,
        output_path: str,
        device_id: str | None = None,
    ) -> None:
        self.address = address
        self.entries_count = entries_count
        self.output_path = output_path
        self.device_id = device_id

    def run_command(self):
        command = self._build_command()
        print(" ".join(command), file=sys.stderr)
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            print(result.stderr, file=sys.stderr)
            sys.exit("Readout failed!")
        else:
            try:
                with open(self.output_path, "xb") as output_file:
                    self._convert_memrd2bin(result.stdout, output_file)
                print("Read out the log!", file=sys.stderr)
            except FileExistsError:
                print(f"Error: The file '{self.output_path}' already exists. Operation aborted.")


    def _convert_memrd2bin(self, infile, outfile):
        lines = infile.splitlines()
        for line in lines:
            entries = line.split(" ")
            entries = entries[1:5]
            print(entries)
            for hex_string in entries:
                h = int(hex_string, 16)
                b = struct.pack('<I', h)
                outfile.write(b)

    def _build_command(self) -> list[str]:
        command = [
            "nrfutil",
            "device",
            "x-read",
            "--width",
            "32",
            "--core",
            "Application",
            "--direct",
        ]
        command += ["--address", self.address]
        command += ["--bytes", "{}".format(self.entries_count * entry_size + header_size)]

        if self.device_id:
            command += ["--serial-number", self.device_id]
        else:
            command += ["--traits", "jlink"]

        return command


class PPR_PRL(WestCommand):
    def __init__(self):
        super().__init__(
            name="ppr-prl",
            help="PPR power request logger",
            description="utility wrappers for ppr power request logger",
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )

        subparsers = parser.add_subparsers(dest="command")
        flash_parser = subparsers.add_parser(
            "flash",
            help="Flash the ppr power logger NVM image",
            formatter_class=argparse.RawDescriptionHelpFormatter
        )
        flash_parser.add_argument("-i", "--image", metavar="PATH", help="Path to NVM image to flash")
        flash_parser.add_argument("--dev-id", help="Device serial number")

        readout_parser = subparsers.add_parser(
            "readout",
            help="Dump the ppr power logger log file",
            formatter_class=argparse.RawDescriptionHelpFormatter
        )
        readout_parser.add_argument("-o", "--output", metavar="PATH", help="Path to the output file", required=True)
        readout_parser.add_argument("--dev-id", help="Device serial number")
        readout_parser.add_argument("--address", help="Starting address of the log buffer to dump", default="0x2fc007e0")
        readout_parser.add_argument("--entries-count", help="Number of log entries", type=int, default=1024)

        return parser

    def do_run(self, args, unknown_args):
        if args.command == "flash":
            self._flash(args)
        if args.command == "readout":
            self._readout(args)
                

    def _flash(self, args: argparse.Namespace) -> None:
        if not args.image:
            nrf_path = self.manifest.topdir + "/nrf"
            image_path = "{}/snippets/haltium_ppr_power_logger/ppr_hex/firmware_nvm_v2.hex".format(nrf_path)
        else:
            image_path = args.image

        runner = NrfutilWrapperFlash(
            device_id=args.dev_id, image_path=image_path
        )
        runner.run_command()

    def _readout(self, args: argparse.Namespace) -> None:
        runner = NrfutilWrapperReadout(
            device_id=args.dev_id, address=args.address, entries_count = args.entries_count, output_path = args.output
        )
        runner.run_command()
