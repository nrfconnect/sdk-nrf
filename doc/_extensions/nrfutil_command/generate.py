"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Generate the snapshot of the ``--json-args`` output of the pinned nrfutil versions.

nrfutil runs in a temporary ``NRFUTIL_HOME``, so the user's nrfutil installation is not changed.
"""

from __future__ import annotations

import json
import os
import subprocess
import tempfile
from pathlib import Path
from typing import Any

from nrfutil_command.specs import Snapshot


class SnapshotError(Exception):
    """nrfutil could not be run, or could not install a pinned version."""


class Nrfutil:
    """Runs nrfutil in a temporary NRFUTIL_HOME."""

    def __init__(self, executable: str = "nrfutil"):
        self.executable = executable
        self._home = tempfile.TemporaryDirectory(prefix="nrfutil-home-")

    def __enter__(self) -> Nrfutil:
        return self

    def __exit__(self, *_exc: object) -> None:
        self._home.cleanup()

    def run(self, args: list[str]) -> str:
        env = dict(os.environ, NRFUTIL_HOME=self._home.name)
        try:
            result = subprocess.run(
                [self.executable, *args],
                check=False,
                env=env,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
            )
        except OSError as e:
            raise SnapshotError(f"Unable to run '{self.executable}': {e}") from e

        if result.returncode != 0:
            output = result.stderr.strip() or result.stdout.strip()
            raise SnapshotError(f"'nrfutil {' '.join(args)}' failed: {output}")
        return result.stdout

    def json_args(self, command: str | None = None) -> dict[str, Any]:
        args = [command, "--json-args"] if command else ["--json-args"]
        stdout = self.run(args)
        try:
            return json.loads(stdout)
        except json.JSONDecodeError as e:
            raise SnapshotError(
                f"Failed to parse the output of 'nrfutil {' '.join(args)}': {e}"
            ) from e

    def core_version(self) -> str:
        """The version of the nrfutil core in NRFUTIL_HOME, from 'nrfutil --version'."""
        first = self.run(["--version"]).split()
        return first[1] if len(first) > 1 else ""


def generate(
    directory: Path,
    core_version: str | None,
    pins: dict[str, str],
    platform: str,
    executable: str = "nrfutil",
) -> Snapshot:
    """Install the pinned nrfutil core and commands and write their json-args to directory.

    Nothing is written until nrfutil has run, so a failed run leaves the previous snapshot in place.
    """
    with Nrfutil(executable) as nrfutil:
        # A new NRFUTIL_HOME bootstraps the latest core, so the pinned one is installed into it
        if core_version:
            nrfutil.run(["self-upgrade", "--to-version", core_version])
        core = nrfutil.json_args()
        commands = {}
        for name, version in sorted(pins.items()):
            nrfutil.run(["install", f"{name}={version}"])
            commands[name] = nrfutil.json_args(name)
        snapshot = Snapshot(
            core_version=core_version or nrfutil.core_version(),
            core=core,
            versions=dict(pins),
            commands=commands,
            platform=platform,
        )

    snapshot.write(directory)
    return snapshot
