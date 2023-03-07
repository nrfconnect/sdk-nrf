#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause


from ecdsa import SigningKey
from ecdsa.curves import NIST256p
from hashlib import sha256
import argparse
import sys


def generate_legal_key():
    """
    Ensure that we don't have 0xFFFF in the hash of the public key of
    the generated keypair.

    :return: A key who's SHA256 digest does not contain 0xFFFF
    """
    while True:
        key = SigningKey.generate(curve=NIST256p)
        digest = sha256(key.get_verifying_key().to_string()).digest()[:16]
        if not (any([digest[n:n + 2] == b'\xff\xff'
                     for n in range(0, len(digest), 2)])):
            return key


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Generate PEM file.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False)

    priv_pub_group = parser.add_mutually_exclusive_group(required=True)
    priv_pub_group.add_argument('--private', required=False, action='store_true',
                                help='Output a private key.')
    priv_pub_group.add_argument('--public', required=False, action='store_true',
                                help='Output a public key.')
    parser.add_argument('--out', '-out', '-o', required=False,
                        type=argparse.FileType('wb'), default=sys.stdout.buffer,
                        help='Output to specified file instead of stdout.')
    parser.add_argument('--in', '-in', '-i', required=False, dest='infile',
                        type=argparse.FileType('rb'),
                        help='Read private key from specified PEM file instead '
                             'of generating it.')

    args = parser.parse_args()

    sk = (SigningKey.from_pem(args.infile.read())
          if args.infile else generate_legal_key())

    if args.private:
        args.out.write(sk.to_pem())

    if args.public:
        vk = sk.get_verifying_key()
        vk.to_pem()
        args.out.write(vk.to_pem())
