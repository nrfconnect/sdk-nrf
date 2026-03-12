#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Hash content of a file.
"""

import argparse
import hashlib
import sys

from intelhex import IntelHex  # type: ignore[import-untyped]

HASH_FUNCTION_FACTORY = {
    'sha256': hashlib.sha256,
    'sha512': hashlib.sha512,
}


def parse_args():
    parser = argparse.ArgumentParser(
        description='Hash data from file.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False)

    parser.add_argument(
        '--infile', '-i', '--in', '-in', required=True,
        help='Hash the contents of the specified file. If a *.hex file is given, the contents will '
             'first be converted to binary, with all non-specified area being set to 0xff. '
             'For all other file types, no conversion is done.'
        '--skip', required=False, default=0, type=lambda x: int(x, 0),
        help='Amount of bytes to skip in front of the input binary, when '
             'generating signature. It can be used to skip space reserved '
             'for header added after firmware is signed.'
    )
    parser.add_argument(
        '--type', '-t', dest='hash_function', help='Hash function (default: %(default)s)',
        action='store', choices=HASH_FUNCTION_FACTORY.keys(), default='sha256'
    )

    return parser.parse_args()


def generate_hash_digest(file: str, skip:int, hash_function_name: str) -> bytes:
    if file.endswith('.hex'):
        ih = IntelHex(file)
        ih.padding = 0xff  # Allows hashing with empty regions
        to_hash = ih.tobinstr()[skip:]
    else:
        to_hash = open(file, 'rb').read()
        to_hash = to_hash[skip:]

    hash_function = HASH_FUNCTION_FACTORY[hash_function_name]
    return hash_function(to_hash).digest()


def main():
    args = parse_args()
    sys.stdout.buffer.write(generate_hash_digest(args.infile, args.skip, args.hash_function))
    return 0


if __name__ == '__main__':
    sys.exit(main())
