# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import json
import os
import sys
from pathlib import Path

import jsonschema
from west.commands import WestCommand

sys.path.append(str(Path(__file__).parent))
import utils


utils.install_json_excepthook()


SCRIPT_DIR = Path(__file__).absolute().parent
SCHEMA = SCRIPT_DIR.parents[2] / "zephyr" / "soc" / "nordic" / "nrf54h" / "bicr" / "bicr-schema.json"


class NcsBICR(WestCommand):
    def __init__(self):
        super().__init__(
            "ncs-bicr",
            "Assist with board BICR creation for any applicable Nordic SoC",
            ""
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )

        parser.add_argument(
            "-d", "--board-dir", type=Path, help="Target board directory"
        )

        group = parser.add_mutually_exclusive_group(required=True)
        group.add_argument(
            "-s", "--json-schema", action="store_true", help="Provide JSON schema"
        )
        group.add_argument(
            "-r", "--json-schema-response", type=str, help="JSON schema response"
        )

        return parser

    def do_run(self, args, unknown_args):
        board_dir = args.board_dir
        if not board_dir:
            board_dir = Path(os.getcwd())

        board_spec = board_dir / "board.yml"
        if not board_spec.exists():
            raise FileNotFoundError("Invalid board directory")

        with open(SCHEMA, "r") as f:
            schema = json.loads(f.read())

        bicr_file = board_dir / "bicr.json"

        bicr = None
        if args.json_schema and bicr_file.exists():
            with open(bicr_file, "r") as f:
                bicr = json.loads(f.read())
        elif args.json_schema_response:
            bicr = json.loads(args.json_schema_response)

        if bicr:
            try:
                jsonschema.validate(bicr, schema)
            except jsonschema.ValidationError as e:
                raise ValueError("Invalid BICR") from e

        if args.json_schema:
            schema = {
                "schema": schema,
                "state": bicr
            }

            print(json.dumps(schema))
        else:
            with open(bicr_file, "w") as f:
                json.dump(bicr, f, indent=4)

            print(json.dumps({"commands": []}))
