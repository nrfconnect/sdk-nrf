#!/usr/bin/env python3
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
"""Validate an OTA bundle and list its payload elements."""

import argparse
import hashlib
import sys
from pathlib import Path

import cbor2

PROTOCOL_VERSION_KEY = 1
BUNDLE_SIZE_KEY = 2
BUNDLE_SHA256_KEY = 3
ELEMENTS_KEY = 4
TARGET_KEY = 5
SIZE_KEY = 6

TARGETS = {
    0: "nrf53_app",
    1: "nrf53_net",
    2: "nrf53_bl",
    3: "nrf91_app",
    4: "nrf91_bl",
    5: "nrf91_mdm_delta",
    6: "nrf91_mdm_full",
}

MAX_SIZE = 2**32 - 1
MAX_ELEMENTS = len(TARGETS)
MAX_HEADER_SIZE = 4096
CHUNK_SIZE = 1024 * 1024


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def check_bundle(path: Path) -> list[tuple[str, int, int]]:
    with path.open("rb") as bundle:
        prefix = bundle.read(MAX_HEADER_SIZE)
        header = None
        payload_offset = 0
        for header_end in range(1, len(prefix) + 1):
            try:
                header = cbor2.loads(prefix[:header_end])
            except (cbor2.CBORDecodeError, EOFError):
                continue
            payload_offset = header_end
            break
        require(header is not None, f"could not decode a complete CBOR header within {MAX_HEADER_SIZE} bytes")
        bundle.seek(payload_offset)

        require(isinstance(header, dict), "header must be a CBOR map")
        require(
            set(header) == {
                PROTOCOL_VERSION_KEY,
                BUNDLE_SIZE_KEY,
                BUNDLE_SHA256_KEY,
                ELEMENTS_KEY,
            },
            "header must contain exactly keys 1, 2, 3, and 4",
        )
        require(
            type(header[PROTOCOL_VERSION_KEY]) is int
            and header[PROTOCOL_VERSION_KEY] == 1,
            "protocol_version must be 1",
        )

        bundle_size = header[BUNDLE_SIZE_KEY]
        require(
            type(bundle_size) is int and 1 <= bundle_size <= MAX_SIZE,
            "bundle_size must be between 1 and 0xffffffff",
        )
        expected_hash = header[BUNDLE_SHA256_KEY]
        require(
            isinstance(expected_hash, bytes) and len(expected_hash) == 32,
            "bundle_sha256 must be a 32-byte byte string",
        )

        elements = header[ELEMENTS_KEY]
        require(isinstance(elements, list), "elements must be an array")
        require(
            1 <= len(elements) <= MAX_ELEMENTS,
            f"elements must contain between 1 and {MAX_ELEMENTS} entries",
        )

        targets = set()
        listed_elements = []
        declared_payload_size = 0
        for index, element in enumerate(elements):
            require(isinstance(element, dict), f"element {index} must be a map")
            require(
                set(element) == {TARGET_KEY, SIZE_KEY},
                f"element {index} must contain exactly keys 5 and 6",
            )

            target_id = element[TARGET_KEY]
            require(
                type(target_id) is int and target_id in TARGETS,
                f"element {index} has an unknown target ID",
            )
            target = TARGETS[target_id]
            require(target not in targets, f"target {target} appears more than once")
            targets.add(target)

            size = element[SIZE_KEY]
            require(
                type(size) is int and 1 <= size <= MAX_SIZE,
                f"element {index} size must be between 1 and 0xffffffff",
            )
            listed_elements.append((target, size, payload_offset + declared_payload_size))
            declared_payload_size += size

        require(
            not {"nrf91_mdm_delta", "nrf91_mdm_full"} <= targets,
            "nrf91_mdm_delta and nrf91_mdm_full cannot be in the same bundle",
        )
        require(
            bundle_size == declared_payload_size,
            "bundle_size must equal the sum of element sizes",
        )

        digest = hashlib.sha256()
        actual_payload_size = 0
        while chunk := bundle.read(CHUNK_SIZE):
            actual_payload_size += len(chunk)
            digest.update(chunk)

    require(
        actual_payload_size == bundle_size,
        f"payload has {actual_payload_size} bytes, expected {bundle_size}",
    )
    require(digest.digest() == expected_hash, "bundle_sha256 does not match the payload")
    return listed_elements


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundle", type=Path, help="OTA bundle file to check")
    args = parser.parse_args()

    try:
        elements = check_bundle(args.bundle)
    except (OSError, ValueError) as error:
        parser.error(str(error))

    print(f"Valid OTA bundle: {args.bundle}")
    print("Elements:")
    for target, size, offset in elements:
        print(f"  {target}: offset {offset}, size {size} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())