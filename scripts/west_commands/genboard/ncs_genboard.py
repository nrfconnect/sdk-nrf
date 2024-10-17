# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from pathlib import Path
import re
import shutil

from jinja2 import Environment, FileSystemLoader
from west.commands import WestCommand
from west import log
from yaml import load

try:
    from yaml import CLoader as Loader
except ImportError:
    from yaml import Loader


SCRIPT_DIR = Path(__file__).absolute().parent
TEMPLATE_DIR = SCRIPT_DIR / "templates"
CONFIG = SCRIPT_DIR / "config.yml"

NCS_VERSION_MIN = (2, 0, 0)
HWMV2_SINCE = (2, 7, 0)
TFM_SINCE = (2, 6, 0)

NCS_VERSION_RE = re.compile(r"^(\d+)\.(\d+)\.(\d+)$")
VENDOR_RE = re.compile(r"^[a-zA-Z0-9_-]+$")
BOARD_RE = re.compile(r"^[a-zA-Z0-9_-]+$")


class NcsGenboard(WestCommand):

    def __init__(self):
        super().__init__(
            "ncs-genboard", "Generate board skeleton files for any Nordic SoC", ""
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )

        parser.add_argument(
            "-o", "--output", required=True, type=Path, help="Output directory"
        )
        parser.add_argument("-e", "--vendor", required=True, help="Vendor name")
        parser.add_argument("-b", "--board", required=True, help="Board name")
        parser.add_argument(
            "-d", "--board-desc", required=True, help="Board description"
        )
        parser.add_argument("-s", "--soc", required=True, help="SoC")
        parser.add_argument("-v", "--variant", required=True, help="Variant")
        parser.add_argument(
            "-n", "--ncs-version", required=True, help="NCS target version"
        )

        return parser

    def do_run(self, args, unknown_args):
        with open(CONFIG, "r") as f:
            config = load(f, Loader=Loader)

        # validate input
        m = NCS_VERSION_RE.match(args.ncs_version)
        if not m:
            log.err(f"Invalid NCS version: {args.ncs_version}")
            return

        ncs_version = tuple(int(n) for n in m.groups())

        if ncs_version < NCS_VERSION_MIN:
            log.err(f"Unsupported NCS version: {args.ncs_version}")
            return

        if not VENDOR_RE.match(args.vendor):
            log.err(f"Invalid vendor name: {args.vendor}")
            return

        if not BOARD_RE.match(args.board):
            log.err(f"Invalid board name: {args.board}")
            return

        series = None
        soc = None
        for product in config["products"]:
            if series:
                break

            for soc_ in product["socs"]:
                if args.soc == soc_["name"]:
                    series = product["series"]
                    soc = soc_
                    break

        if not series:
            log.err(f"Invalid/unsupported SoC: {args.soc}")
            return

        targets = []
        for variant in soc["variants"]:
            if args.variant == variant["name"]:
                since = variant.get("since", ".".join(str(i) for i in NCS_VERSION_MIN))
                m = NCS_VERSION_RE.match(since)
                if not m:
                    log.err(f"Malformed NCS version: {since}")
                    return

                since_version = tuple(int(n) for n in m.groups())
                if ncs_version < since_version:
                    log.err(
                        f"SoC {args.soc}-{args.variant} not supported in NCS version {args.ncs_version}"
                    )
                    return

                if "cores" in variant:
                    for core in variant["cores"]:
                        target = {
                            "core": core["name"],
                            "ram": core["ram"],
                            "flash": core["flash"],
                            "ns": core.get("ns", False) if ncs_version >= TFM_SINCE else False,
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
                        "ns": variant.get("ns", False) if ncs_version >= TFM_SINCE else False,
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
            log.err(f"Invalid/unsupported variant: {args.variant}")
            return

        # prepare Jinja environment
        env = Environment(
            trim_blocks=True,
            lstrip_blocks=True,
            loader=FileSystemLoader(TEMPLATE_DIR / series),
        )

        env.globals["ncs_version"] = ncs_version
        env.globals["hwmv2_since"] = HWMV2_SINCE
        env.globals["vendor"] = args.vendor
        env.globals["board"] = args.board
        env.globals["board_desc"] = args.board_desc
        env.globals["series"] = series
        env.globals["soc"] = args.soc
        env.globals["variant"] = args.variant
        env.globals["targets"] = targets

        # render templates/copy files
        if ncs_version < HWMV2_SINCE:
            out_dir = args.output / "arm" / args.board
        else:
            out_dir = args.output / args.vendor / args.board

        if not out_dir.exists():
            out_dir.mkdir(parents=True)

        # board-level files
        tmpl = TEMPLATE_DIR / series / "pre_dt_board.cmake"
        shutil.copy(tmpl, out_dir)

        tmpl = TEMPLATE_DIR / series / "board-pinctrl.dtsi"
        shutil.copy(tmpl, out_dir / f"{ args.board }-pinctrl.dtsi")

        tmpl = env.get_template("board.cmake.jinja2")
        with open(out_dir / "board.cmake", "w") as f:
            f.write(tmpl.render())

        tmpl = env.get_template("Kconfig.board.jinja2")
        fname = (
            "Kconfig.board" if ncs_version < HWMV2_SINCE else f"Kconfig.{args.board}"
        )
        with open(out_dir / fname, "w") as f:
            f.write(tmpl.render())

        if ncs_version >= HWMV2_SINCE:
            tmpl = env.get_template("board.yml.jinja2")
            with open(out_dir / f"board.yml", "w") as f:
                f.write(tmpl.render())

        tmpl = env.get_template("Kconfig.defconfig.jinja2")
        with open(out_dir / f"Kconfig.defconfig", "w") as f:
            f.write(tmpl.render(config))

        # nrf53 specific files
        if series == "nrf53":
            tmpl = env.get_template("board-cpuapp_partitioning.dtsi.jinja2")
            with open(out_dir / f"{ args.board }-cpuapp_partitioning.dtsi", "w") as f:
                f.write(tmpl.render(config))

            tmpl = TEMPLATE_DIR / series / "board-shared_sram.dtsi"
            shutil.copy(tmpl, out_dir / f"{ args.board }-shared_sram.dtsi")

        # nrf91 specific files
        if series == "nrf91":
            tmpl = env.get_template("board-partitioning.dtsi.jinja2")
            with open(out_dir / f"{ args.board }-partitioning.dtsi", "w") as f:
                f.write(tmpl.render(config))

        # per-target files
        for target in targets:
            name = args.board
            if target.get("core"):
                if ncs_version >= HWMV2_SINCE:
                    name += f"_{args.soc}"
                name += f"_{target['core']}"
            if target["ns"]:
                name += "_ns"
            elif target["xip"]:
                name += "_xip"

            tmpl = env.get_template("board_defconfig.jinja2")
            with open(out_dir / f"{ name }_defconfig", "w") as f:
                f.write(tmpl.render(target=target))

            tmpl = env.get_template("board.dts.jinja2")
            with open(out_dir / f"{name}.dts", "w") as f:
                f.write(tmpl.render(target=target))

            tmpl = env.get_template("board_twister.yml.jinja2")
            with open(out_dir / f"{name}.yml", "w") as f:
                f.write(tmpl.render(target=target))
