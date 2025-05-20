#!/usr/bin/env python3
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import base64
import difflib
import hashlib
import lzma
import os
import shutil
from contextlib import suppress
from dataclasses import asdict, dataclass, replace
from datetime import datetime, timezone
from pathlib import Path
from functools import reduce

import yaml
from colorama import Fore, Style
from west.commands import WestCommand

CONFIG = {
    "template_path": "{nrf}/config/suit/templates",
    "metadata_name": "metadata.yaml",
    "editable_dir": "suit"
}


@dataclass
class MetadataEntry:
    content: str
    digest: str
    name: str


@dataclass
class Source:
    soc: str
    variant: str


@dataclass
class Metadata:
    create_date: str
    update_date: str
    source: Source
    entries: list[MetadataEntry]


class SuitManifest(WestCommand):
    def __init__(self):
        super(SuitManifest, self).__init__(
            "suit-manifest",
            "SUIT manifest",
            "SUIT manifest utility tool.",
        )
        self.template_path = CONFIG["template_path"]
        self.metadata_name = CONFIG["metadata_name"]
        self.editable_dir = CONFIG["editable_dir"]

    def create_metadata_entry(self, name: str, manifest: str) -> MetadataEntry:
        digest = hashlib.sha256(manifest.encode("utf-8"))
        compressed = lzma.compress(manifest.encode("utf-8"))
        content = base64.b64encode(compressed).decode("utf-8")
        return MetadataEntry(content=content, digest=digest.hexdigest(), name=name)

    def get_current_date(self) -> str:
        now = datetime.now(timezone.utc)
        return now.replace(microsecond=0).isoformat()

    def create_metadata(self, source: Source, entries: list[MetadataEntry]) -> Metadata:
        create_date = self.get_current_date()
        return Metadata(
            create_date=create_date,
            update_date=create_date,
            source=source,
            entries=entries,
        )

    def get_content(self, metadata: MetadataEntry) -> str:
        decoded = base64.b64decode(metadata.content.encode("utf-8"))
        decompressed = lzma.decompress(decoded)
        return decompressed.decode("utf-8")

    def deserialize_entry(self, data: dict[str, str]) -> MetadataEntry:
        return MetadataEntry(
            content=data["content"], digest=data["digest"], name=data["name"]
        )

    def deserialize_source(self, data: dict[str, str]) -> Source:
        return Source(soc=data["soc"], variant=data["variant"])

    def load_metadata(self, file: Path) -> Metadata:
        with open(file, "r") as f:
            data = yaml.safe_load(f.read())
            entries = [
                self.deserialize_entry(entry_data) for entry_data in data["entries"]
            ]
            source = self.deserialize_source(data["source"])
            return Metadata(
                create_date=data["create_date"],
                update_date=data["update_date"],
                source=source,
                entries=entries,
            )

    def dump_metadata(self, metadata: Metadata, file: Path) -> None:
        with open(file, "w") as f:
            yaml.dump(asdict(metadata), f)

    def color_diff_line(self, line: str) -> str:
        code = line[0]
        color = Style.RESET_ALL
        if code == "-":
            color = Fore.RED
        elif code == "+":
            color = Fore.GREEN
        return f"{color}{line}"

    def color_diff(self, lines: list[str]) -> list[str]:
        return [self.color_diff_line(line) for line in lines]

    def get_source_dir(self, source: Source) -> Path:
        return self.get_template_path() / source.soc / source.variant

    def get_metadata_path(self, soc: str, base_dir: Path) -> Path:
        return base_dir / self.editable_dir / soc / self.metadata_name

    def copy_manifests(self, source: Source, output_dir: Path) -> list[MetadataEntry]:
        source_dir = self.get_source_dir(source)
        entries = []
        for file in source_dir.glob("*.yaml.jinja2"):
            dest_file = output_dir / "suit" / source.soc / file.name
            with suppress(FileExistsError):
                os.makedirs(dest_file.parent)
            shutil.copy(file, dest_file)
            with open(file, "r") as f:
                data = f.read()
                entry = self.create_metadata_entry(file.name, data)
                entries.append(entry)
        return entries

    def check_entry(self, source: Source, entry: MetadataEntry) -> bool:
        source_dir = self.get_source_dir(source)
        source_file = source_dir / entry.name
        with open(source_file, "r") as f:
            data = f.read()
            digest = hashlib.sha256(data.encode("utf-8"))
            return digest.hexdigest() == entry.digest

    def diff_entry(self, source: Source, entry: MetadataEntry) -> list[str]:
        source_dir = self.get_source_dir(source)
        source_file = source_dir / entry.name
        content = self.get_content(entry).split("\n")
        with open(source_file, "r") as f:
            data = f.read().split("\n")
            return difflib.unified_diff(
                content, data, fromfile=entry.name, tofile=entry.name
            )

    def make_diffs(self, metadata: Metadata) -> list[str]:
        diffs = [self.diff_entry(metadata.source, entry) for entry in metadata.entries]
        colored = [self.color_diff(diff) for diff in diffs]
        return ["\n".join(diff) for diff in colored]

    def accept_review(self, metadata: Metadata) -> Metadata:
        source_dir = self.get_source_dir(metadata.source)
        update_date = self.get_current_date()
        entries = []
        for entry in metadata.entries:
            source_file = source_dir / entry.name
            with open(source_file, "r") as f:
                data = f.read()
                entry = self.create_metadata_entry(entry.name, data)
                entries.append(entry)
        new_metadata = replace(metadata, entries=entries, update_date=update_date)
        return new_metadata

    def get_template_path(self) -> Path:
        projects = self.manifest.projects
        name_and_abspath = map(lambda x: {x.name: x.abspath}, projects)
        project_paths = reduce(lambda x, y: x | y, name_and_abspath)
        if "nrf" not in project_paths:
            project_paths["nrf"] = project_paths["manifest"]
        return Path(self.template_path.format(**project_paths))

    def fix_source_version(self, source: Source) -> Source:
        variant, version = source.variant.split("/")
        variant_dir = self.get_template_path() / source.soc / variant
        fixed_version = version
        if version == "latest":
            # Latest version is found by the directory name in format v1, v2, ...
            latest_dir = max(variant_dir.iterdir(), key=lambda x: int(x.name[1:]))
            fixed_version = latest_dir.name
        fixed_variant = f"{variant}/{fixed_version}"
        return replace(source, variant=fixed_variant)

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )

        subparsers = parser.add_subparsers(
            title="init",
            description="init subcommands",
            help="Review manifest changes",
            dest="command",
        )
        init_parser = subparsers.add_parser("init", help="Initialize custom manifest.")
        init_parser.add_argument(
            "-o", "--app-dir", type=Path, help="Output directory", default=Path.cwd()
        )
        init_parser.add_argument("-s", "--soc", type=str, help="SoC", required=True)
        init_parser.add_argument(
            "-v",
            "--variant",
            type=str,
            help="Manifest template variant",
            default="default/latest",
        )

        review = subparsers.add_parser("review", help="Review manifest.")
        review.add_argument(
            "-i", "--app-dir", type=Path, help="Application directory", default=Path.cwd()
        )
        review.add_argument(
            "-s", "--soc", type=Path, help="SoC", required=True
        )
        review.add_argument(
            "-a", "--accept", action="store_true", help="Accept changes", default=False
        )

        check = subparsers.add_parser("check", help="Review manifest.")
        check.add_argument(
            "-i", "--app-dir", type=Path, help="Application directory", default=Path.cwd()
        )
        check.add_argument(
            "-s", "--soc", type=Path, help="SoC", required=True
        )
        return parser

    def do_run(self, args, unknown_args):
        if args.command == "init":
            source = Source(soc=args.soc, variant=args.variant)
            source = self.fix_source_version(source)
            entries = self.copy_manifests(source, args.app_dir)
            metadata = self.create_metadata(source, entries)
            destination_path = self.get_metadata_path(source.soc, args.app_dir)
            self.dump_metadata(metadata, destination_path)
        elif args.command == "review":
            metadata_path = self.get_metadata_path(args.soc, args.app_dir)
            metadata = self.load_metadata(metadata_path)
            if not args.accept:
                for diff in filter(lambda x: x, self.make_diffs(metadata)):
                    print(diff)
            else:
                new_metadata = self.accept_review(metadata)
                self.dump_metadata(new_metadata, metadata_path)
        elif args.command == "check":
            metadata_path = self.get_metadata_path(args.soc, args.app_dir)
            metadata = self.load_metadata(metadata_path)
            for entry in metadata.entries:
                if not self.check_entry(metadata.source, entry):
                    print(f"{entry.name} is out of date")
