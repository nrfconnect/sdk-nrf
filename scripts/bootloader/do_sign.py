#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
from __future__ import annotations

import argparse
import contextlib
import hashlib
import sys
from pathlib import Path
from typing import BinaryIO, Generator

from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives.serialization import load_pem_private_key
from ecdsa.keys import SigningKey  # type: ignore[import-untyped]
from intelhex import IntelHex  # type: ignore[import-untyped]


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description='Sign data from stdin or file.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False
    )

    parser.add_argument('-k', '--private-key', required=True, type=Path,
                        help='Private key to use.')
    parser.add_argument('-i', '--in', '-in', required=False, dest='infile',
                        type=Path, default=sys.stdin.buffer,
                        help='Sign the contents of the specified file instead of stdin.')
    parser.add_argument('-o', '--out', '-out', required=False, dest='outfile',
                        type=Path, default=None,
                        help='Write the signature to the specified file instead of stdout.')
    parser.add_argument(
        '--algorithm', '-a', dest='algorithm',
        help='Signing algorithm (default: %(default)s)',
        action='store', choices=['ecdsa', 'ed25519'], default='ecdsa',
    )

    args = parser.parse_args(argv)

    return args


@contextlib.contextmanager
def open_stream(output_file: Path | None = None) -> Generator[BinaryIO, None, None]:
    if output_file is not None:
        stream = open(output_file, 'wb')
        try:
            yield stream
        finally:
            stream.close()
    else:
        yield sys.stdout.buffer


def hex_to_binary(input_hex_file: str) -> bytes:
    ih = IntelHex(input_hex_file)
    ih.padding = 0xff  # Allows hashing with empty regions
    data = ih.tobinstr()
    return data


def sign_with_ecdsa(
    private_key_file: Path, input_file: Path, output_file: Path | None = None
) -> int:
    with open(private_key_file, 'r') as f:
        private_key = SigningKey.from_pem(f.read())
    with open(input_file, 'rb') as f:
        data = f.read()
    signature = private_key.sign(data, hashfunc=hashlib.sha256)
    with open_stream(output_file) as stream:
        stream.write(signature)
    return 0


def sign_with_ed25519(
    private_key_file: Path, input_file: Path, output_file: Path | None = None
) -> int:
    with open(private_key_file, 'rb') as f:
        private_key: Ed25519PrivateKey = load_pem_private_key(f.read(), password=None)  # type: ignore[assignment]
    if str(input_file).endswith('.hex'):
        data = hex_to_binary(str(input_file))
    else:
        with open(input_file, 'rb') as f:
            data = f.read()
    signature = private_key.sign(data)
    with open_stream(output_file) as stream:
        stream.write(signature)
    return 0


def main(argv=None) -> int:
    args = parse_args(argv)
    if args.algorithm == 'ecdsa':
        return sign_with_ecdsa(args.private_key, args.infile, args.outfile)
    if args.algorithm == 'ed25519':
        return sign_with_ed25519(args.private_key, args.infile, args.outfile)
    return 1


if __name__ == '__main__':
    sys.exit(main())
