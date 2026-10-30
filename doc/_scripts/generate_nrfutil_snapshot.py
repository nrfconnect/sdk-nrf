"""
Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

Generate the snapshot of the ``--json-args`` output of the pinned nrfutil versions, which the
``nrfutil-command`` directive validates commands against.

The documentation build generates the snapshot by itself when the pinned versions change, so this
script is only needed to generate it in another directory or with other versions, for example to
check a new nrfutil release before pinning it.

The versions come from ``scripts/tools-versions-<os>.yml``, the ``nrfutil_<command>_ver``
substitutions in ``doc/nrf/shortcuts.txt`` and ``doc/_extensions/nrfutil_command/pins.yml``.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

NCS_ROOT = Path(__file__).resolve().parents[2]

sys.path.insert(0, str(NCS_ROOT / "doc" / "_extensions"))

from nrfutil_command.generate import SnapshotError, generate  # noqa: E402
from nrfutil_command.specs import (  # noqa: E402
    host_platform,
    load_core_version,
    load_pinned_versions,
)

DEFAULT_OUTPUT = NCS_ROOT / "doc" / "_build" / "nrfutil-specs"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog=__file__,
        description="Generate the json-args snapshot of the pinned nrfutil versions",
        allow_abbrev=False,
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=NCS_ROOT,
        help="Root of the sdk-nrf repository, to read the pinned versions from",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help="Directory to write the snapshot to. Default: %(default)s",
    )
    parser.add_argument("--nrfutil", default="nrfutil", help="nrfutil executable to use")
    parser.add_argument(
        "--pin",
        action="append",
        default=[],
        metavar="COMMAND=VERSION",
        help="Version of an nrfutil command to install, overriding the pinned versions. "
        "Can be repeated.",
    )
    args = parser.parse_args(argv)

    platform = host_platform()
    pins = load_pinned_versions(args.root, platform)
    for pin in args.pin:
        command, sep, version = pin.partition("=")
        if not sep or not command or not version:
            parser.error(f"--pin must have the form COMMAND=VERSION, got '{pin}'")
        pins[command] = version

    try:
        snapshot = generate(
            args.output, load_core_version(args.root, platform), pins, platform, args.nrfutil
        )
    except SnapshotError as e:
        print(f"ERROR: {e}")
        return 1

    print(f"Wrote the {platform} json-args of nrfutil {snapshot.core_version} to {args.output}")
    for name, version in sorted(snapshot.versions.items()):
        print(f"  {name} {version}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
