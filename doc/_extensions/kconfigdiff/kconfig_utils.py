#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import argparse
import io
import logging
import os
import re
import sys
from collections import defaultdict
from collections.abc import Generator
from itertools import chain
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import TypeAlias

import requests
from dotenv import load_dotenv
from sphinx.util.display import progress_message

from . import pickler

sys.path.insert(0, str(Path(__file__).parents[4] / "zephyr/scripts"))
sys.path.insert(0, str(Path(__file__).parents[4] / "zephyr/scripts/kconfig"))

import kconfiglib
import list_boards
import list_hardware
import zephyr_module

logger = logging.Logger(__name__)
RESOURCES_DIR = Path(__file__).parent / "static"
ZEPHYR_BASE = Path(__file__).parents[4] / "zephyr"
NRF_BASE = Path(__file__).parents[3]

KCONFIG_SAVE_FILE = "kconfig.zip"
KCONFIG_URL = f"https://ncsdoc.z6.web.core.windows.net/ncs/{{version}}/kconfig/{KCONFIG_SAVE_FILE}"

MAX_ENTRY_LINE_CHANGE = 5

SC: TypeAlias = kconfiglib.Symbol | kconfiglib.Choice


def kconfig_load(
    ext_kconfigs: list[Path | str],
) -> tuple[kconfiglib.Kconfig, kconfiglib.Kconfig, dict[str, str]]:
    """Load Kconfig"""
    with TemporaryDirectory() as td:
        modules = zephyr_module.parse_modules(ZEPHYR_BASE)

        # generate Kconfig.modules file
        kconfig_module_dirs = ""
        kconfig = ""
        sysbuild_kconfig = ""
        for module in modules:
            kconfig_module_dirs += zephyr_module.process_kconfig_module_dir(
                module.project, module.meta, False
            )
            kconfig += zephyr_module.process_kconfig(module.project, module.meta)
            sysbuild_kconfig += zephyr_module.process_sysbuildkconfig(module.project, module.meta)

        with open(Path(td) / "kconfig_module_dirs.env", "w") as f:
            f.write(kconfig_module_dirs)

        with open(Path(td) / "Kconfig.modules", "w") as f:
            f.write(kconfig)

        with open(Path(td) / "Kconfig.sysbuild.modules", "w") as f:
            f.write(sysbuild_kconfig)

        # generate dummy Kconfig.dts file
        kconfig = ""

        with open(Path(td) / "Kconfig.dts", "w") as f:
            f.write(kconfig)

        (Path(td) / 'soc').mkdir(exist_ok=True)
        root_args = argparse.Namespace(**{'soc_roots': [Path(ZEPHYR_BASE)]})
        v2_systems = list_hardware.find_v2_systems(root_args)

        soc_folders = {soc.folder[0] for soc in v2_systems.get_socs()}
        with open(Path(td) / "soc" / "Kconfig.defconfig", "w") as f:
            f.write('')

        with open(Path(td) / "soc" / "Kconfig.soc", "w") as f:
            for folder in soc_folders:
                f.write('source "' + (Path(folder) / 'Kconfig.soc').as_posix() + '"\n')

                if "nordic" in folder:
                    f.write('osource "' + (Path(folder) / 'Kconfig.sysbuild').as_posix() + '"\n')

        with open(Path(td) / "soc" / "Kconfig", "w") as f:
            for folder in soc_folders:
                f.write('osource "' + (Path(folder) / 'Kconfig').as_posix() + '"\n')

        (Path(td) / 'arch').mkdir(exist_ok=True)
        root_args = argparse.Namespace(**{'arch_roots': [Path(ZEPHYR_BASE)], 'arch': None})
        v2_archs = list_hardware.find_v2_archs(root_args)
        kconfig = ""
        for arch in v2_archs['archs']:
            kconfig += 'source "' + (Path(arch['path']) / 'Kconfig').as_posix() + '"\n'
        with open(Path(td) / "arch" / "Kconfig", "w") as f:
            f.write(kconfig)

        (Path(td) / 'boards').mkdir(exist_ok=True)
        root_args = argparse.Namespace(
            **{
                'board_roots': [Path(ZEPHYR_BASE)],
                'soc_roots': [Path(ZEPHYR_BASE)],
                'board': None,
                'board_dir': [],
            }
        )
        v2_boards = list_boards.find_v2_boards(root_args).values()

        with open(Path(td) / "boards" / "Kconfig.boards", "w") as f:
            for board in v2_boards:
                board_str = 'BOARD_' + re.sub(r"[^a-zA-Z0-9_]", "_", board.name).upper()
                f.write('config  ' + board_str + '\n')
                f.write('\t bool\n')
                for qualifier in list_boards.board_v2_qualifiers(board):
                    board_str = 'BOARD_' + re.sub(r"[^a-zA-Z0-9_]", "_", qualifier).upper()
                    f.write('config  ' + board_str + '\n')
                    f.write('\t bool\n')
                f.write('source "' + (board.dir / ('Kconfig.' + board.name)).as_posix() + '"\n\n')

        # base environment
        os.environ["ZEPHYR_BASE"] = str(ZEPHYR_BASE)
        os.environ["srctree"] = str(ZEPHYR_BASE)  # noqa: SIM112
        os.environ["KCONFIG_DOC_MODE"] = "1"
        os.environ["KCONFIG_BINARY_DIR"] = td

        # include all archs and boards
        os.environ["ARCH_DIR"] = "arch"
        os.environ["ARCH"] = "[!v][!2]*"
        os.environ["HWM_SCHEME"] = "v2"

        os.environ["BOARD"] = "boards"
        os.environ["KCONFIG_BOARD_DIR"] = str(Path(td) / "boards")
        load_dotenv(str(Path(td) / "kconfig_module_dirs.env"))

        # Sysbuild runs first
        os.environ["CONFIG_"] = "SB_CONFIG_"
        sysbuild_output = kconfiglib.Kconfig(ZEPHYR_BASE / "share" / "sysbuild" / "Kconfig")

        # Normal Kconfig runs second
        os.environ["CONFIG_"] = "CONFIG_"

        # insert external Kconfigs to the environment
        module_paths = dict()
        for module in modules:
            name = module.meta["name"]
            name_var = module.meta["name-sanitized"].upper()
            module_paths[name] = module.project

            build_conf = module.meta.get("build")
            if not build_conf:
                continue

            # Module Kconfig file has already been specified
            if f"ZEPHYR_{name_var}_KCONFIG" in os.environ:
                continue

            if build_conf.get("kconfig"):
                kconfig = Path(module.project) / build_conf["kconfig"]
                os.environ[f"ZEPHYR_{name_var}_KCONFIG"] = str(kconfig)
            elif build_conf.get("kconfig-ext"):
                for path in ext_kconfigs:
                    # Assume that the kconfig file exists at this path.
                    # Technically the cmake variable can be constructed arbitarily
                    # by "{ext_path}/modules/modules.cmake"
                    kconfig = Path(path) / "modules" / name / "Kconfig"
                    if kconfig.exists():
                        os.environ[f"ZEPHYR_{name_var}_KCONFIG"] = str(kconfig)

        return kconfiglib.Kconfig(ZEPHYR_BASE / "Kconfig"), sysbuild_output, module_paths


def is_path_in_workspace(zephyr_dir: str, path) -> bool:
    path = os.path.normpath(path)
    zephyr_dir = os.path.normpath(Path(zephyr_dir).parent)
    return os.path.commonpath([path, zephyr_dir]) == zephyr_dir


def get_path_rel2node(node: SC, path) -> str:
    orig_zephyr_base = node.kconfig.srctree
    return path if not os.path.isabs(path) else os.path.relpath(path, orig_zephyr_base)


def kconfig_node_generator(
    kconfigs: list[kconfiglib.Kconfig],
) -> Generator[tuple[kconfiglib.MenuNode, SC], None, None]:
    for kconfig_obj in kconfigs:
        os.environ["CONFIG_"] = kconfig_obj.config_prefix
        for sc in sorted(
            chain(kconfig_obj.unique_defined_syms, kconfig_obj.unique_choices),
            key=lambda sc: sc.name if sc.name else "",
        ):
            if not sc.name:
                continue

            dedup: set[str] = set()
            for node in filter(lambda node: node.prompt or node.help, sc.nodes):
                if (path := f"{node.filename}:{node.linenr}") in dedup:
                    continue
                dedup.add(path)
                yield node, sc


def entry_name(sc) -> str:
    prefix = sc.kconfig.config_prefix if sc.kconfig else ""
    return f"{prefix}{sc.name}"


class KconfigEntryProperties:
    __slots__ = (
        "_node",
        "_sc",
        "_kconfiglib",
        "name",
        "prompt",
        "type",
        "help",
        "dependencies",
        "defaults",
        "selects",
        "selected_by",
        "implies",
        "implied_by",
        "ranges",
        "choices",
    )

    def __init__(self, node: kconfiglib.MenuNode, sc: SC, kconfiglib_=None):
        self._node = node
        self._sc = sc

        self._kconfiglib = kconfiglib

        if kconfiglib_:
            self._kconfiglib = kconfiglib_

        self.name = entry_name(sc)
        self.prompt = node.prompt[0] if node.prompt else ""
        self.type = self._kconfiglib.TYPE_TO_STR[sc.type]
        self.help = node.help
        self.dependencies = self._init_deps()
        self.defaults = self._init_defaults()
        self.selects = self._init_selects()
        self.selected_by = self._init_selected_by()
        self.implies = self._init_implies()
        self.implied_by = self._init_implied_by()
        self.ranges = self._init_ranges()
        self.choices = self._init_choices()

    def sc_fmt(self, sc):
        prefix = os.environ["CONFIG_"]

        if isinstance(sc, self._kconfiglib.Symbol):
            if sc.nodes:
                return f'{prefix}{sc.name}'

            if sc.is_constant and sc.name not in self._kconfiglib.STR_TO_TRI:
                formatted = self._kconfiglib.escape(sc.name)
                if os.path.isabs(formatted) and is_path_in_workspace(
                    self._node.kconfig.srctree, formatted
                ):
                    formatted = get_path_rel2node(self._node, formatted)
                return f'"{formatted}"'

            return sc.name

        if not sc.name:
            return "&ltchoice&gt"
        return f'&ltchoice {prefix}{sc.name}&gt'

    def _init_deps(self) -> list[str]:
        deps = []
        if self._node.dep is not self._sc.kconfig.y:
            deps = self._kconfiglib.expr_str(self._node.dep, self.sc_fmt)
        return deps

    def _init_defaults(self) -> list[str]:
        defaults = []
        for value, cond in self._node.orig_defaults:
            fmt = self._kconfiglib.expr_str(value, self.sc_fmt)
            if cond is not self._sc.kconfig.y:
                fmt += f" if {self._kconfiglib.expr_str(cond, self.sc_fmt)}"
            defaults.append(fmt)
        return defaults

    def _init_selects(self) -> list[str]:
        selects = []
        for value, cond in self._node.orig_selects:
            fmt = self._kconfiglib.expr_str(value, self.sc_fmt)
            if cond is not self._sc.kconfig.y:
                fmt += f" if {self._kconfiglib.expr_str(cond, self.sc_fmt)}"
            selects.append(fmt)
        return selects

    def _init_selected_by(self) -> list[str]:
        selected_by = []
        if isinstance(self._sc, self._kconfiglib.Symbol) and self._sc.rev_dep != self._sc.kconfig.n:
            for select in self._kconfiglib.split_expr(self._sc.rev_dep, self._kconfiglib.OR):
                sym = self._kconfiglib.split_expr(select, self._kconfiglib.AND)[0]
                selected_by.append(f"{entry_name(sym)}")
        return selected_by

    def _init_ranges(self) -> list[str]:
        ranges = list()
        for min_, max_, cond in self._node.orig_ranges:
            fmt = (
                f"[{self._kconfiglib.expr_str(min_, self.sc_fmt)}, "
                f"{self._kconfiglib.expr_str(max_, self.sc_fmt)}]"
            )
            if cond is not self._sc.kconfig.y:
                fmt += f" if {self._kconfiglib.expr_str(cond, self.sc_fmt)}"
            ranges.append(fmt)
        return ranges

    def _init_choices(self) -> list[str]:
        choices = []
        if isinstance(self._sc, self._kconfiglib.Choice):
            for sym in self._sc.syms:
                choices.append(self._kconfiglib.expr_str(sym, self.sc_fmt))
        return choices

    def _init_implies(self) -> list[str]:
        implies = []
        for value, cond in self._node.orig_implies:
            fmt = self._kconfiglib.expr_str(value, self.sc_fmt)
            if cond is not self._sc.kconfig.y:
                fmt += f" if {self._kconfiglib.expr_str(cond, self.sc_fmt)}"
            implies.append(fmt)
        return implies

    def _init_implied_by(self) -> list[str]:
        implied_by = []
        if (
            isinstance(self._sc, self._kconfiglib.Symbol)
            and self._sc.weak_rev_dep != self._sc.kconfig.n
        ):
            for select in self._kconfiglib.split_expr(self._sc.weak_rev_dep, self._kconfiglib.OR):
                sym = self._kconfiglib.split_expr(select, self._kconfiglib.AND)[0]
                implied_by.append(f"{entry_name(sym)}")
        return implied_by

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, KconfigEntryProperties):
            raise NotImplementedError

        return (
            self.name == other.name
            and self.prompt == other.prompt
            and self.type == other.type
            and self.help == other.help
            and self.dependencies == other.dependencies
            and set(self.defaults) == set(other.defaults)
            and set(self.selects) == set(other.selects)
            and set(self.selected_by) == set(other.selected_by)
            and set(self.implies) == set(other.implies)
            and set(self.implied_by) == set(other.implied_by)
            and set(self.ranges) == set(other.ranges)
            and set(self.choices) == set(other.choices)
        )


def fetch_prev_kconfig_file(version) -> tuple[kconfiglib.Kconfig, kconfiglib.Kconfig, kconfiglib]:
    url = KCONFIG_URL.format(version=version)
    with progress_message(f"Fetching kconfig from previous build, {url=}"):
        res = requests.get(url)
        res.raise_for_status()

        with io.BytesIO(res.content) as f:
            return pickler.load_file(f)


def diff_generator(
    prev_version,
    outdir: Path,
) -> Generator[tuple[KconfigEntryProperties | None, KconfigEntryProperties | None], None, None]:
    """Yield ``(old, new)`` pairs of :class:`KconfigEntryProperties`.

    Either side may be ``None`` when an entry was added or removed.
    """

    def node_uid(node: kconfiglib.MenuNode, sc: SC) -> str:
        return f"{entry_name(sc)}/{get_path_rel2node(node, node.filename)}"

    new_map: dict[str, list[tuple[kconfiglib.MenuNode, kconfiglib.Symbol | kconfiglib.Choice]]] = (
        defaultdict(list)
    )

    try:
        kconfig_old, sysbuild_kconfig_old, old_kconfiglib = fetch_prev_kconfig_file(prev_version)
    except requests.exceptions.RequestException:
        logger.error("Failed to fetch old kconfig")
        return

    with progress_message("Generating kconfig diff pairs"):
        kconfig, sysbuild_kconfig, _ = kconfig_load([ZEPHYR_BASE, NRF_BASE])

        for node, sc in kconfig_node_generator([kconfig, sysbuild_kconfig]):
            new_map[node_uid(node, sc)].append((node, sc))

        for node, sc in kconfig_node_generator([kconfig_old, sysbuild_kconfig_old]):
            if new_items := new_map.get(node_uid(node, sc)):
                to_del = None
                if len(new_items) == 1:
                    to_del = 0
                else:
                    for i, (new_node, _) in enumerate(new_items):
                        if abs(new_node.linenr - node.linenr) < MAX_ENTRY_LINE_CHANGE:
                            to_del = i
                            break
                    else:
                        yield KconfigEntryProperties(node, sc, old_kconfiglib), None
                        continue
                new_node, new_sc = new_items.pop(to_del)
                yield (
                    KconfigEntryProperties(node, sc, old_kconfiglib),
                    KconfigEntryProperties(new_node, new_sc),
                )
                continue

        for node, sc in chain(*new_map.values()):
            yield None, KconfigEntryProperties(node, sc)

        pickler.save_kconfig(outdir / KCONFIG_SAVE_FILE, kconfig, sysbuild_kconfig)
