#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic


from ecdsa import SigningKey
from ecdsa.curves import NIST256p
import argparse
import sys


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate PEM file.",
        formatter_class=argparse.RawDescriptionHelpFormatter)

    priv_pub_group = parser.add_mutually_exclusive_group(required=True)
    priv_pub_group.add_argument("--private", required=False, action="store_true",
                                help="Output a private key.")
    priv_pub_group.add_argument("--public", required=False, action="store_true",
                                help="Output a public key.")
    parser.add_argument("--out", "-out", "-o", required=False,
                        type=argparse.FileType('wb'), default=sys.stdout.buffer,
                        help="Output to specified file instead of stdout.")
    parser.add_argument("--in", "-in", "-i", required=False, dest="infile",
                        type=argparse.FileType('rb'),
                        help="Read private key from specified PEM file instead of generating it.")

    args = parser.parse_args()

    sk = SigningKey.from_pem(args.infile.read()) if args.infile else SigningKey.generate(curve=NIST256p)

    if args.private:
        args.out.write(sk.to_pem())

    if args.public:
        vk = sk.get_verifying_key()
        vk.to_pem()
        args.out.write(vk.to_pem())
