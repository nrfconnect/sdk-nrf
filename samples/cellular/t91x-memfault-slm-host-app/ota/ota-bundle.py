#!/usr/bin/env python3
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
"""Create an OTA bundle as described by ota-bundle.cddl.

The bundle is a CBOR header (ota_bundle) followed by the element payloads, in the order given,
with no padding. The header contains the sum of the element sizes and the SHA-256 of the
concatenated payloads (the header is excluded from both).

Example:
    ota-bundle.py -o bundle.bin nrf91_app=app_update.bin nrf91_mdm_full=mfw.bin
"""

import argparse
import hashlib
import sys
from pathlib import Path

import cbor2

# Keys of the ota_bundle and ota_element maps
PROTOCOL_VERSION_KEY = 1
BUNDLE_SIZE_KEY = 2
BUNDLE_SHA256_KEY = 3
ELEMENTS_KEY = 4
TARGET_KEY = 5
SIZE_KEY = 6

PROTOCOL_VERSION = 1

TARGETS = {
    "nrf53_app": 0,
    "nrf53_net": 1,
    "nrf91_app": 2,
    "nrf91_mdm_delta": 3,
    "nrf91_mdm_full": 4,
}

MAX_SIZE = 2**32 - 1


def parse_element(arg: str) -> tuple[str, Path]:
    target, sep, path = arg.partition("=")
    if not sep or not path:
        raise argparse.ArgumentTypeError(f"'{arg}' is not of the form <target>=<file>")
    if target not in TARGETS:
        raise argparse.ArgumentTypeError(
            f"unknown target '{target}', expected one of: {', '.join(TARGETS)}"
        )
    return target, Path(path)


def build_bundle(elements: list[tuple[str, Path]]) -> bytes:
    targets = [target for target, _ in elements]

    if len(set(targets)) != len(targets):
        raise ValueError("each target may only appear once")
    if "nrf91_mdm_delta" in targets and "nrf91_mdm_full" in targets:
        raise ValueError("nrf91_mdm_delta and nrf91_mdm_full cannot be in the same bundle")

    payloads = [path.read_bytes() for _, path in elements]
    for (target, path), payload in zip(elements, payloads):
        if not payload:
            raise ValueError(f"{path} ({target}) is empty")
        if len(payload) > MAX_SIZE:
            raise ValueError(f"{path} ({target}) does not fit in 32 bits")

    payload = b"".join(payloads)
    if len(payload) > MAX_SIZE:
        raise ValueError("bundle size does not fit in 32 bits")

    header = {
        PROTOCOL_VERSION_KEY: PROTOCOL_VERSION,
        BUNDLE_SIZE_KEY: len(payload),
        BUNDLE_SHA256_KEY: hashlib.sha256(payload).digest(),
        ELEMENTS_KEY: [
            {TARGET_KEY: TARGETS[target], SIZE_KEY: len(data)}
            for target, data in zip(targets, payloads)
        ],
    }
    return cbor2.dumps(header) + payload


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("elements", nargs="+", type=parse_element, metavar="TARGET=FILE")
    parser.add_argument("-o", "--output", type=Path, required=True, help="output bundle file")
    args = parser.parse_args()

    if len(args.elements) > len(TARGETS):
        parser.error(f"at most {len(TARGETS)} elements are allowed")

    try:
        bundle = build_bundle(args.elements)
    except (ValueError, OSError) as e:
        parser.error(str(e))

    args.output.write_bytes(bundle)
    print(f"Wrote {args.output} ({len(bundle)} bytes, {len(args.elements)} elements)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
