#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic


import hashlib
import sys
import argparse


def parse_args():
    parser = argparse.ArgumentParser(
        description="Hash data from stdin or file.",
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("--infile", "-i", "--in", "-in", required=False,
                        type=argparse.FileType('rb'), default=sys.stdin.buffer,
                        help="Hash the contents of the specified file instead of stdin.")

    args = parser.parse_args()

    return args


if __name__ == "__main__":
    args = parse_args()
    sys.stdout.buffer.write(hashlib.sha256(args.infile.read()).digest())
