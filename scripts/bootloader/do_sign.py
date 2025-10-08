#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
from __future__ import annotations

import argparse
import contextlib
import sys
from collections.abc import Generator
from pathlib import Path
from typing import BinaryIO

from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.ec import EllipticCurvePrivateKey
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.hazmat.primitives.asymmetric.utils import decode_dss_signature
from cryptography.hazmat.primitives.serialization import load_pem_private_key
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
    with open(private_key_file, 'rb') as f:
        private_key = load_pem_private_key(f.read(), password=None)
    if not isinstance(private_key, EllipticCurvePrivateKey):
        raise SystemExit(f'Private key file {private_key_file} is not Elliptic Curve key')

    with open(input_file, 'rb') as f:
        data = f.read()
    signature = private_key.sign(data, ec.ECDSA(hashes.SHA256()))
    asn1_signature_bytes = parse_asn1_signature(signature)
    with open_stream(output_file) as stream:
        stream.write(asn1_signature_bytes)
    return 0


def parse_asn1_signature(signature: bytes, length=32) -> bytes:
    asn1_decoded: tuple[int, int] = decode_dss_signature(signature)
    asn1_signature_bytes = b''.join([
        b.to_bytes(length, byteorder='big') for b in asn1_decoded
    ])
    assert len(asn1_signature_bytes) == 2 * length
    return asn1_signature_bytes


def sign_with_ed25519(
    private_key_file: Path, input_file: Path, output_file: Path | None = None
) -> int:
    with open(private_key_file, 'rb') as f:
        private_key = load_pem_private_key(f.read(), password=None)
    if not isinstance(private_key, Ed25519PrivateKey):
        raise SystemExit(f'Private key file {private_key_file} is not Ed25519 key')

    if str(input_file).endswith('.hex'):
        data = hex_to_binary(str(input_file))
    else:
        with open(input_file, 'rb') as f:
            data = f.read()
    signature = private_key.sign(data)
    with open_stream(output_file) as stream:
        stream.write(signature)
    return 0


ALGORITHMS = {
    'ecdsa': sign_with_ecdsa,
    'ed25519': sign_with_ed25519,
}


def main(argv=None) -> int:
    args = parse_args(argv)
    sign_function = ALGORITHMS[args.algorithm]
    return sign_function(args.private_key, args.infile, args.outfile)


if __name__ == '__main__':
    sys.exit(main())
