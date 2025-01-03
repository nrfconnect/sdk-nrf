# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import json
import os
import re
import sys
from pathlib import Path

from west.commands import WestCommand

sys.path.append(str(Path(__file__).parent))
import utils


utils.install_json_excepthook()


SCRIPT_DIR = Path(__file__).absolute().parent
BICRGEN = (
    SCRIPT_DIR.parents[2]
    / "zephyr"
    / "soc"
    / "nordic"
    / "nrf54h"
    / "bicr"
    / "bicrgen.py"
)


def soc_series_detect(board_dir):
    found_defconfig = None
    for defconfig in board_dir.glob("Kconfig.*"):
        if defconfig.name == "Kconfig.defconfig":
            continue

        found_defconfig = defconfig
        break

    if not found_defconfig:
        raise FileNotFoundError("Board defconfig file not found")

    with open(found_defconfig, "r") as f:
        for line in f.readlines():
            m = re.match(".*select SOC_(NRF[0-9]{2}[A-Z]{,1}).*", line)
            if not m:
                continue

            return m.group(1).lower()

    raise ValueError("Unsupported board")


class NcsBoardActions(WestCommand):
    def __init__(self):
        super().__init__(
            "ncs-board-actions", "Obtain available actions for any board", ""
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )

        parser.add_argument(
            "-d",
            "--board-dir",
            type=Path,
            help="Target board directory (defaults to current working directory)",
        )

        return parser

    def do_run(self, args, unknown_args):
        board_dir = args.board_dir
        if not board_dir:
            board_dir = Path(os.getcwd())

        board_spec = board_dir / "board.yml"
        if not board_spec.exists():
            raise FileNotFoundError("Invalid board directory")

        series = soc_series_detect(board_dir)
        commands = []

        if series == "nrf54h":
            commands.extend(
                [
                    {
                        "name": "Create/Edit BICR",
                        "command": "west",
                        "args": ["ncs-bicr", "--board-dir", str(board_dir.resolve())],
                        "properties": {
                            "providesJsonSchema": True,
                        }
                    },
                    {
                        "name": "Generate BICR binary",
                        "command": sys.executable,
                        "args": [
                            str(BICRGEN),
                            "--svd",
                            "${SVD_FILE}",
                            "--input",
                            str((board_dir / "bicr.json").resolve()),
                            "--output",
                            "${BICR_HEX_FILE}",
                        ],
                        "properties": {
                            "providesJsonSchema": False,
                        }
                    },
                ]
            )

        print(json.dumps({"commands": commands}))
