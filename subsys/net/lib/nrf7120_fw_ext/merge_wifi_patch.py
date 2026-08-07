#!/usr/bin/env python3
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Merge nRF7120 Wi-Fi ROM patches into application HEX files.

Combines one or more application HEX inputs (e.g. zephyr.hex, or TF-M
secure + non-secure HEX files) with Wi-Fi firmware patch binaries into a
single merged HEX and BIN suitable for MCUboot signing or direct flashing.

Each patch binary is placed at a fixed MRAM origin address declared in the
Wi-Fi firmware manifest. The merge rejects any address overlap between
inputs and produces a combined HEX and BIN output.

Exit codes:
    0  Success
    1  Error (missing input, overlap, bad arguments)
"""

import argparse
import sys
from pathlib import Path

from intelhex import AddressOverlapError, IntelHex


def parse_int(value: str) -> int:
    """Parse a decimal or hex (0x-prefixed) integer string."""
    return int(value, 0)


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument(
        "--input-hex",
        required=True,
        action="append",
        type=Path,
        help="Application HEX file to merge. May be repeated for TF-M "
        "builds where secure and non-secure images are separate.",
    )
    parser.add_argument(
        "--output-hex", required=True, type=Path, help="Path for the merged Intel HEX output."
    )
    parser.add_argument(
        "--output-bin", required=True, type=Path, help="Path for the merged raw binary output."
    )
    parser.add_argument(
        "--patch",
        action="append",
        nargs=4,
        required=True,
        metavar=("NAME", "BIN", "ORIGIN", "OUT_HEX"),
        help="Wi-Fi patch to merge: symbolic name, path to binary, "
        "MRAM origin address, and per-patch HEX output path. "
        "May be repeated for multiple patches (e.g. lmac, umac).",
    )

    return parser.parse_args(argv)


def main() -> None:
    args = parse_args()
    script_name = Path(__file__).name

    args.output_hex.parent.mkdir(parents=True, exist_ok=True)
    args.output_bin.parent.mkdir(parents=True, exist_ok=True)

    # Validate inputs exist before doing any work.
    for path in args.input_hex:
        if not path.is_file():
            sys.exit(f"ERROR: input HEX not found: {path}")

    # Merge application HEX inputs.
    merged = IntelHex(str(args.input_hex[0]))
    merged.start_addr = None

    try:
        for path in args.input_hex[1:]:
            image = IntelHex(str(path))
            image.start_addr = None
            merged.merge(image, overlap="error")

        # Merge each Wi-Fi patch binary at its declared origin.
        for name, bin_path, origin_str, out_hex in args.patch:
            binary = Path(bin_path)
            origin = parse_int(origin_str)
            output = Path(out_hex)

            if not binary.is_file():
                sys.exit(f"ERROR: {name} patch binary not found: {binary}")

            data = binary.read_bytes()

            patch = IntelHex()
            patch.frombytes(data, offset=origin)
            patch.write_hex_file(str(output))
            merged.merge(patch, overlap="error")

            print(
                f"{script_name}: {name}: {len(data)} B @ 0x{origin:X} -> {output.name}",
                file=sys.stderr,
            )

    except AddressOverlapError as exc:
        sys.exit(f"ERROR: address overlap during merge: {exc}")

    # Write outputs.
    merged.padding = 0xFF
    merged.write_hex_file(str(args.output_hex))
    merged.tobinfile(str(args.output_bin), start=merged.minaddr(), end=merged.maxaddr())

    print(f"{script_name}: app: {' + '.join(p.name for p in args.input_hex)}", file=sys.stderr)
    print(f"{script_name}: out: {args.output_hex.name}, {args.output_bin.name}", file=sys.stderr)


if __name__ == "__main__":
    main()
