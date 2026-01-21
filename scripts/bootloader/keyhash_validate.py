#!/usr/bin/env python3
#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from __future__ import annotations

import argparse
import sys
from hashlib import sha256, sha512

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.serialization import load_pem_public_key


def check_elliptic_curve(infile):
    """
    Ensure that we don't have 0xFFFF in the hash of the public key.
    """
    key = load_pem_public_key(infile.read())
    public_bytes = key.public_bytes(
            encoding=serialization.Encoding.X962,
            format=serialization.PublicFormat.UncompressedPoint,
        )
    digest = sha256(public_bytes[1:]).digest()[:16]
    if not (any([digest[n:n + 2] == b'\xff\xff'
                 for n in range(0, len(digest), 2)])):
        print("EC key hash ok ", digest.hex())
        return 0
    return 1


def check_ed25519(infile):
    """
    Ensure that we don't have 0xFFFF in the hash of the public key.
    """
    key = load_pem_public_key(infile.read())
    public_bytes = key.public_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PublicFormat.SubjectPublicKeyInfo,
    )
    digest = sha512(public_bytes[1:]).digest()[:16]
    if not any([digest[n:n + 2] == b'\xff\xff' for n in range(0, len(digest), 2)]):
        print("ED25519 key hash ok ", digest.hex())
        return 0
    return 1

def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description='Check PEM file.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False
    )

    parser.add_argument('--in', '-in', '-i', required=True, dest='infile',
                        type=argparse.FileType('rb'),
                        help='Read public key from specified PEM file.')
    parser.add_argument(
        '--algorithm', '-a', help='Signing algorithm (default: %(default)s)',
        required=False, action='store', choices=('ec', 'ed25519'), default='ec'
    )

    args = parser.parse_args(argv)
    if args.algorithm == "ec":
        return check_elliptic_curve(args.infile)
    else:
        return check_ed25519(args.infile)


if __name__ == '__main__':
    sys.exit(main())
