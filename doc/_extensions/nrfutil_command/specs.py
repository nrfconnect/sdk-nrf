"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Pinned nrfutil versions and the snapshot of their ``--json-args`` output.

The snapshot lets the documentation build validate nrfutil commands without running nrfutil for
every command. It is generated from the pinned versions by :mod:`nrfutil_command.generate` and is
not committed.
"""

from __future__ import annotations

import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import yaml

from nrfutil_command.walker import ValidationError

PACKAGE_DIR = Path(__file__).resolve().parent
PINS_FILE = PACKAGE_DIR / "pins.yml"
COMMAND_URLS_FILE = PACKAGE_DIR / "command-urls.json"

MANIFEST_FILE = "manifest.json"
CORE_SPEC_FILE = "nrfutil.json"

TOOLS_VERSIONS_FILES = {
    "linux": "tools-versions-linux.yml",
    "macos": "tools-versions-darwin.yml",
    "windows": "tools-versions-win10.yml",
}

# Operating systems of the tabs whose commands are validated only on that operating system
TAB_PLATFORMS = {"Windows": "windows", "Linux": "linux", "macOS": "macos"}

# Values of the :os: option of the nrfutil-command directive and the operating systems they select
OS_OPTIONS = {
    "windows": {"windows"},
    "linux": {"linux"},
    "macos": {"macos"},
    "unix": {"linux", "macos"},
}

SHORTCUT_VERSION_RE = re.compile(
    r"^\.\.\s+\|\w*?nrfutil_(?P<command>[\w-]+)_ver\|\s+replace::\s*(?P<version>\S+)\s*$",
    re.MULTILINE,
)


def host_platform() -> str:
    """The operating system that the documentation is built on, named like TAB_PLATFORMS."""
    if sys.platform == "win32":
        return "windows"
    return "macos" if sys.platform == "darwin" else "linux"


def _load_tools_versions_nrfutil(ncs_root: Path, platform: str) -> dict[str, Any]:
    path = ncs_root / "scripts" / TOOLS_VERSIONS_FILES.get(platform, TOOLS_VERSIONS_FILES["linux"])
    if not path.is_file():
        return {}
    data = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    return data.get("nrfutil") or {}


def load_core_version(ncs_root: Path, platform: str | None = None) -> str | None:
    """Return the nrfutil core version pinned in scripts/tools-versions-<os>.yml, if any."""
    version = _load_tools_versions_nrfutil(ncs_root, platform or host_platform()).get("version")
    return str(version) if version else None


def load_pinned_versions(
    ncs_root: Path, platform: str | None = None, pins_file: Path | None = None
) -> dict[str, str]:
    """Return the nrfutil command versions to install, keyed by command.

    Versions in scripts/tools-versions-<os>.yml take precedence over the ``nrfutil_<command>_ver``
    substitutions in doc/nrf/shortcuts.txt, which document the versions used by the guides. Both
    take precedence over pins.yml, which pins the commands that neither of them pins.
    """
    pins = {}

    pins_file = pins_file or PINS_FILE
    if pins_file.is_file():
        data = yaml.safe_load(pins_file.read_text(encoding="utf-8")) or {}
        pins.update({str(command): str(version) for command, version in data.items()})

    shortcuts = ncs_root / "doc" / "nrf" / "shortcuts.txt"
    if shortcuts.is_file():
        for match in SHORTCUT_VERSION_RE.finditer(shortcuts.read_text(encoding="utf-8")):
            pins[match.group("command")] = match.group("version")

    tools_versions = _load_tools_versions_nrfutil(ncs_root, platform or host_platform())
    subcommands = tools_versions.get("subcommands") or {}
    pins.update({str(command): str(version) for command, version in subcommands.items()})

    return pins


def _read_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as e:
        raise ValidationError(f"Failed to read '{path}': {e}") from e


def strip_descriptions(data: Any) -> Any:
    """Return the json-args without the help texts, which the walker doesn't use."""
    if isinstance(data, dict):
        return {k: strip_descriptions(v) for k, v in data.items() if k != "description"}
    if isinstance(data, list):
        return [strip_descriptions(v) for v in data]
    return data


def _write_json(path: Path, data: Any) -> None:
    path.write_text(
        json.dumps(data, indent=1, sort_keys=True) + "\n", encoding="utf-8", newline="\n"
    )


@dataclass
class Snapshot:
    """The --json-args output of nrfutil core and of the pinned nrfutil commands."""

    core_version: str | None
    core: dict[str, Any]
    versions: dict[str, str] = field(default_factory=dict)
    commands: dict[str, dict[str, Any]] = field(default_factory=dict)
    # Operating system the json-args come from, because they differ between operating systems
    platform: str = "linux"

    @classmethod
    def load(cls, directory: Path) -> Snapshot:
        manifest_path = directory / MANIFEST_FILE
        if not manifest_path.is_file():
            raise ValidationError(f"No nrfutil command snapshot in '{directory}'")

        manifest = _read_json(manifest_path)
        versions = {str(k): str(v) for k, v in (manifest.get("commands") or {}).items()}
        return cls(
            core_version=manifest.get("core"),
            core=_read_json(directory / CORE_SPEC_FILE),
            versions=versions,
            commands={name: _read_json(directory / f"{name}.json") for name in versions},
            platform=manifest.get("platform", "linux"),
        )

    def write(self, directory: Path) -> None:
        """Write the snapshot. The manifest goes last, so an interrupted write has none."""
        directory.mkdir(parents=True, exist_ok=True)
        for old in directory.glob("*.json"):
            old.unlink()
        _write_json(directory / CORE_SPEC_FILE, strip_descriptions(self.core))
        for name, program in self.commands.items():
            _write_json(directory / f"{name}.json", strip_descriptions(program))
        _write_json(
            directory / MANIFEST_FILE,
            {"core": self.core_version, "commands": self.versions, "platform": self.platform},
        )

    def matches(self, core_version: str | None, pins: dict[str, str], platform: str) -> bool:
        """Whether the snapshot holds exactly the pinned versions for the given platform."""
        return (
            self.platform == platform
            and self.versions == pins
            and (core_version is None or core_version == self.core_version)
        )

    def program(self, subcommand_path: list[str]) -> dict[str, Any]:
        """Return the --json-args of the nrfutil command at the given path."""
        if len(subcommand_path) != 1:
            raise ValidationError(
                f"Unexpected subcommand path {subcommand_path}, expected exactly one command"
            )
        name = subcommand_path[0]
        if name not in self.commands:
            raise ValidationError(
                f"Unknown nrfutil command '{name}'. If it exists, pin its version in "
                "doc/_extensions/nrfutil_command/pins.yml."
            )
        return self.commands[name]


def load_command_urls(path: Path = COMMAND_URLS_FILE) -> tuple[str, dict[str, str]]:
    """Return the base URL and the page of each command path from the nrfutil-rs URL map."""
    if not path.is_file():
        return "", {}
    data = _read_json(path)
    return data.get("base_url", ""), dict(data.get("commands") or {})


def command_url(base_url: str, urls: dict[str, str], subcommand_path: list[str]) -> str | None:
    """Return the URL of the most specific documented command that the path starts with."""
    for length in range(len(subcommand_path), 0, -1):
        page = urls.get(" ".join(subcommand_path[:length]))
        if page:
            return base_url + page
    return None
