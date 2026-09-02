# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
West command responsible for merging hex files based on a mergehex.yaml file.
The file should be in the following format:

    output_name:
    - path/to/file1.hex
    - file2.hex

Example:

    merged_thingy91x_nrf9151_s_ns.hex:
    - merged_thingy91x_nrf9151_ns.hex
    - merged_thingy91x_nrf9151.hex
    merged.hex:
    - signed_by_b0_mcuboot.hex
    - b0/zephyr/zephyr.hex
    - signed_by_b0_mcuboot_s1_variant.hex
    - app/zephyr/zephyr.signed.hex
    - app_provision.hex

The provided paths should be relative to the build directory.
"""

import os
import sys
from argparse import ArgumentParser, BooleanOptionalAction, Namespace, RawDescriptionHelpFormatter
from collections.abc import Callable
from functools import cached_property
from pathlib import Path
from subprocess import CalledProcessError
from typing import Any, TypeAlias, cast, override

import yaml
from intelhex import IntelHexError
from jsonschema import ValidationError, validate
from west.commands import WestCommand

_MERGEHEX_FILENAME = "mergehex.yaml"

MergeConfig: TypeAlias = dict[str, list[str]]

_SCHEMA = {
    "type": "object",
    "propertyNames": {"type": "string", "minLength": 1},
    "additionalProperties": {"type": "array", "items": {"type": "string"}},
}


class NcsMergeHex(WestCommand):
    def __init__(self) -> None:
        super().__init__(
            name="ncs-mergehex",
            help="Merge sysbuild output hex files based on mergehex.yaml file.",
            description=__doc__,
        )

    @override
    def do_add_parser(self, parser_adder: Any) -> ArgumentParser:
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            description=self.description,
            formatter_class=RawDescriptionHelpFormatter,
        )

        parser.add_argument(
            "-d",
            "--build-dir",
            help="Path to build directory.",
            type=Path,
        )

        parser.add_argument(
            "--rebuild",
            help="Rebuild if necessary (default True).",
            action=BooleanOptionalAction,
            default=True,
        )

        parser.add_argument(
            "--fail-on-missing",
            help="Fail if any file is missing for one of the configurations.",
            action="store_true",
            default=False,
        )

        parser.add_argument(
            "mergehex_file",
            help=(
                "Path to mergehex.yaml file. "
                "By default a file named mergehex.yaml in either current "
                "directory or the built application's source directory will be "
                "looked up. Otherwise a path needs to be provided."
            ),
            type=Path,
            nargs="?",
        )

        return cast(ArgumentParser, parser)

    @override
    def do_run(self, args: Namespace, _: list[str]) -> None:
        self._insert_paths()

        build_dir = args.build_dir or self._get_build_dir()
        mergehex_file = args.mergehex_file or self._get_mergehex_file(build_dir / "build_info.yml")

        if args.rebuild:
            self._rebuild(str(build_dir))

        config = self._load_config(mergehex_file)

        self.inf("---- Merging files ----")
        merged_any = False
        for output, inputs in config.items():
            output_path = build_dir / output

            existing, missing = self._split_out_missing(inputs, build_dir)

            if args.fail_on_missing and missing:
                self.die(
                    f"Not enough files to produce {output}, missing files: {', '.join(missing)}."
                )

            if not existing:
                self.dbg(f"Couldn't find any files for {output}, skipping")
                continue

            merged_any = True

            output_dir = output_path.parent
            output_dir.mkdir(parents=True, exist_ok=True)

            self.inf(f"[*] Generating {output_path} from {self._format_input_source(existing)}")
            try:
                self._merge_hex_files(output_path, existing)
            except IntelHexError as e:
                self.die(str(e))

            self.inf("Merged file generated.")

        if not merged_any:
            self.die(f"Couldn't find files matching any configuration from {mergehex_file}.")

    def _rebuild(self, build_dir: str) -> None:
        try:
            self._z_run_build(build_dir)
        except CalledProcessError:
            self.die(f"Failed to run build in: {build_dir}")

    def _load_config(self, path: str | os.PathLike[str]) -> MergeConfig:
        try:
            with open(path, "rb") as f:
                config = yaml.safe_load(f)
            validate(config, schema=_SCHEMA)
            return cast(MergeConfig, config)
        except yaml.YAMLError:
            self.die(f"Error parsing yaml in {path}")
        except OSError:
            self.die(f"File {path} doesn't exist or couldn't have been opened.")
        except ValidationError as e:
            self.die(f"Failed parsing mergehex.yaml file:\n{e.message}")

    def _get_build_dir(self) -> Path:
        if self._is_zephyr_build("."):
            return Path(".")
        if (path := Path("build/")).exists() and self._is_zephyr_build(str(path)):
            return path
        self.die("Couldn't find build dir, provide one using --build-dir.")

    def _split_out_missing(self, inputs: list[str], build_dir: Path) -> tuple[list[str], list[str]]:
        existing = []
        missing = []

        for file in inputs:
            path = build_dir / file
            if path.is_file():
                existing.append(str(path))
                continue
            missing.append(str(path))

        return existing, missing

    def _insert_paths(self) -> None:
        env_base = os.getenv("ZEPHYR_BASE")
        if not env_base:
            self.die("ZEPHYR_BASE is not set")
        zephyr_base = Path(env_base)

        sys.path.insert(0, str(zephyr_base / "scripts" / "west_commands"))
        sys.path.insert(0, str(zephyr_base / "scripts" / "build"))

    @cached_property
    def _is_zephyr_build(self) -> Callable[[str], bool]:
        from build_helpers import is_zephyr_build

        return is_zephyr_build

    @cached_property
    def _z_run_build(self) -> Callable[[str], None]:
        from zcmake import run_build

        return run_build

    @cached_property
    def _merge_hex_files(self) -> Callable[[str | os.PathLike[str], list[str]], None]:
        from mergehex import merge_hex_files

        return lambda output, inputs: merge_hex_files(output, inputs, "error", False)

    def _locate_app_dir(self, build_info_path: Path) -> Path | None:
        if not build_info_path.is_file():
            return None

        build_info = yaml.safe_load(build_info_path.read_text())
        images = build_info.get("cmake", {}).get("images", []) or []
        main: dict[str, Any] = next(
            filter(lambda image: image.get("type", "") == "MAIN", images), {}
        )
        if "source-dir" not in main:
            return None

        return Path(main.get("source-dir"))

    def _get_mergehex_file(self, build_info_path: Path) -> Path:
        cwd = Path.cwd()
        path = cwd / _MERGEHEX_FILENAME
        if path.is_file():
            return path

        app_dir = self._locate_app_dir(build_info_path)
        if app_dir and (merge_hex := app_dir / _MERGEHEX_FILENAME).is_file():
            return merge_hex

        self.die("Couldn't find mergehex.yaml file.")

    @staticmethod
    def _format_input_source(inputs: list[str]) -> str:
        if len(inputs) > 1:
            return ", ".join(inputs[:-1]) + " and " + inputs[-1]
        return inputs[0]
