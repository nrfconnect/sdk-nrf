# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import json
import shutil
import sys
from pathlib import Path

import jsonschema
from jinja2 import Environment, FileSystemLoader, TemplateNotFound
from west.commands import WestCommand
from yaml import load

try:
    from yaml import CLoader as Loader
except ImportError:
    from yaml import Loader

sys.path.append(str(Path(__file__).parents[1]))
import utils

utils.install_json_excepthook()


SCRIPT_DIR = Path(__file__).absolute().parent
TEMPLATE_DIR = SCRIPT_DIR / "templates"
CONFIG = SCRIPT_DIR / "config.yml"
SCHEMA = SCRIPT_DIR / "schema.json"


class NcsCreateBoard(WestCommand):

    def __init__(self):
        super().__init__(
            "ncs-create-board", "Create board skeleton files for any Nordic SoC", ""
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
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
        with open(SCHEMA) as f:
            schema = json.loads(f.read())

        if args.json_schema:
            schema = {
                "schema": schema,
                "state": None,
            }

            print(json.dumps(schema))
            return

        with open(CONFIG) as f:
            config = load(f, Loader=Loader)

        # validate input
        input = json.loads(args.json_schema_response)

        try:
            jsonschema.validate(input, schema)
        except jsonschema.ValidationError as e:
            raise ValueError("Board configuration is not valid") from e

        soc_parts = input["soc"].split("-")
        req_soc = soc_parts[0].lower()
        req_variant = soc_parts[1].lower()

        series = None
        soc = None
        for product in config["products"]:
            if series:
                break

            for soc_ in product["socs"]:
                if req_soc == soc_["name"]:
                    series = product["series"]
                    soc = soc_
                    break

        if not series:
            raise ValueError(f"Invalid/unsupported SoC: {req_soc}")

        targets = []
        for variant in soc["variants"]:
            if req_variant == variant["name"]:
                if "cores" in variant:
                    for core in variant["cores"]:
                        target = {
                            "core": core["name"],
                            "ram": core["ram"],
                            "flash": core["flash"],
                            "ns": core.get("ns", False),
                            "xip": core.get("xip", False),
                            "arch": core.get("arch", "arm"),
                        }

                        targets.append(target)
                        if target["ns"]:
                            target_ns = target.copy()
                            target_ns["ns"] = False
                            targets.append(target_ns)
                        elif target["xip"]:
                            target_xip = target.copy()
                            target_xip["xip"] = False
                            targets.append(target_xip)
                else:
                    target = {
                        "ram": variant["ram"],
                        "flash": variant["flash"],
                        "ns": variant.get("ns", False),
                        "xip": variant.get("xip", False),
                        "arch": variant.get("arch", "arm"),
                    }

                    targets.append(target)
                    if target["ns"]:
                        target_ns = target.copy()
                        target_ns["ns"] = False
                        targets.append(target_ns)

                break

        if not targets:
            raise ValueError(f"Invalid/unsupported variant: {req_variant}")

        # prepare Jinja environment
        env = Environment(
            trim_blocks=True,
            lstrip_blocks=True,
            loader=FileSystemLoader(TEMPLATE_DIR / series),
        )

        env.globals["vendor"] = input["vendor"]
        env.globals["board"] = input["board"]
        env.globals["board_desc"] = input["description"]
        env.globals["series"] = series
        env.globals["soc"] = req_soc
        env.globals["variant"] = req_variant
        env.globals["targets"] = targets

        # render templates/copy files
        out_dir = Path(input["root"]) / "boards" / input["vendor"] / input["board"]
        if not out_dir.exists():
            out_dir.mkdir(parents=True)

        # board-level files
        tmpl = TEMPLATE_DIR / series / "pre_dt_board.cmake"
        shutil.copy(tmpl, out_dir)

        tmpl = TEMPLATE_DIR / series / "board-pinctrl.dtsi"
        shutil.copy(tmpl, out_dir / f"{ input['board'] }-pinctrl.dtsi")

        tmpl = env.get_template("board.cmake.jinja2")
        with open(out_dir / "board.cmake", "w") as f:
            f.write(tmpl.render())

        tmpl = env.get_template("Kconfig.board.jinja2")
        with open(out_dir / f"Kconfig.{input['board']}", "w") as f:
            f.write(tmpl.render())

        tmpl = env.get_template("board.yml.jinja2")
        with open(out_dir / "board.yml", "w") as f:
            f.write(tmpl.render())

        try:
            tmpl = env.get_template("Kconfig.defconfig.jinja2")
            with open(out_dir / "Kconfig.defconfig", "w") as f:
                f.write(tmpl.render(config))
        except TemplateNotFound:
            pass

        # nrf53 specific files
        if series == "nrf53":
            tmpl = env.get_template("board-cpuapp_partitioning.dtsi.jinja2")
            with open(out_dir / f"{ input['board'] }-cpuapp_partitioning.dtsi", "w") as f:
                f.write(tmpl.render(config))

            tmpl = TEMPLATE_DIR / series / "board-shared_sram.dtsi"
            shutil.copy(tmpl, out_dir / f"{ input['board'] }-shared_sram.dtsi")

        # nrf91 specific files
        if series == "nrf91":
            tmpl = env.get_template("board-partitioning.dtsi.jinja2")
            with open(out_dir / f"{ input['board'] }-partitioning.dtsi", "w") as f:
                f.write(tmpl.render(config))

        # per-target files
        for target in targets:
            name = input["board"]
            if target.get("core"):
                name += f"_{req_soc}_{target['core']}"
            if target["ns"]:
                name += "_ns"
            if target["xip"]:
                name += "_xip"

            tmpl = env.get_template("board_defconfig.jinja2")
            with open(out_dir / f"{ name }_defconfig", "w") as f:
                f.write(tmpl.render(target=target))

            tmpl = env.get_template("board.dts.jinja2")
            with open(out_dir / f"{name}.dts", "w") as f:
                f.write(tmpl.render(target=target))

            tmpl = env.get_template("board_twister.yaml.jinja2")
            with open(out_dir / f"{name}.yaml", "w") as f:
                f.write(tmpl.render(target=target))

        # return post-commands
        commands = []
        print(json.dumps({"commands": commands}))
