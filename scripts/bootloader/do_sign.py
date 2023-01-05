#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause


import sys
import argparse
import hashlib
from ecdsa import SigningKey


def parse_args():
    parser = argparse.ArgumentParser(
        description='Sign data from stdin or file.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False)

    parser.add_argument('-k', '--private-key', required=True, type=argparse.FileType('rb'),
                        help='Private key to use.')
    parser.add_argument('-i', '--in', '-in', required=False, dest='infile',
                        type=argparse.FileType('rb'), default=sys.stdin.buffer,
                        help='Sign the contents of the specified file instead of stdin.')
    parser.add_argument('-o', '--out', '-out', required=False, dest='outfile',
                        type=argparse.FileType('wb'), default=sys.stdout.buffer,
                        help='Write the signature to the specified file instead of stdout.')

    args = parser.parse_args()

    return args


if __name__ == '__main__':
    args = parse_args()
    private_key = SigningKey.from_pem(args.private_key.read())
    data = args.infile.read()
    signature = private_key.sign(data, hashfunc=hashlib.sha256)
    args.outfile.write(signature)
