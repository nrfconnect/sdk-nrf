#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic


from ecdsa import SigningKey
from ecdsa.curves import NIST256p
import argparse


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate PEM file.",
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--output-private", required=False,
                        help="Output private key file name.")
    parser.add_argument("--output-public", required=False,
                        help="Output public key file name.")
    args = parser.parse_args()

    sk = SigningKey.generate(curve=NIST256p)
    if args.output_private:
        with open(args.output_private, 'wb') as sk_file:
            sk_file.write(sk.to_pem())

    if args.output_public:
        vk = sk.get_verifying_key()
        vk.to_pem()
        with open(args.output_public, 'wb') as vk_file:
            vk_file.write(vk.to_pem())






