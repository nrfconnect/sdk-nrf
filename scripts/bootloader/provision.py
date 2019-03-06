#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic


from intelhex import IntelHex

import argparse
import struct
from ecdsa import SigningKey
from ecdsa import VerifyingKey
from hashlib import sha256

VERBOSE = False


def verbose_print(printstr):
    if VERBOSE:
        print(printstr)


def generate_provision_hex_file(s0_address, s1_address, hashes, provision_address, output):
    # Add addresses
    provision_data = struct.pack('III', s0_address, s1_address, len(hashes))
    provision_data += b''.join(hashes)

    ih = IntelHex()
    ih.frombytes(provision_data, offset=provision_address)
    ih.write_hex_file(output)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate provision hex file.",
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--s0-addr", type=lambda x: int(x,0), required=True, help="Set address of s0 explicitly")
    parser.add_argument("--s1-addr", type=lambda x: int(x,0), required=True, help="Set address of s1 explicitly")
    parser.add_argument("--provision-addr", type=lambda x: int(x,0), required=True, help="Set address of provision data explicitly")
    parser.add_argument("--public-key-files", required=True,
                        help="Semicolon-separated list of public key .pem files.")
    parser.add_argument("-o", "--output", required=False, default="provision.hex",
                        help="Output file name.")
    parser.add_argument("-v", "--verbose", required=False, action="count",
                        help="Verbose mode.")
    return parser.parse_args()


def get_hashes(public_key_files):
    hashes = list()
    for fn in public_key_files:
        verbose_print("Getting hash of %s" % fn)
        with open(fn, 'rb') as f:
            hashes.append(sha256(VerifyingKey.from_pem(f.read()).to_string()).digest()[:16])
        verbose_print("hash: " + hashes[-1].hex())

    if len(hashes) != len(set(hashes)):
        raise RuntimeError("Duplicate public key found. Note that the public key corresponding to the private"
                           "key used for signing is automatically added, and must not be added explicitly.")

    return hashes


def main():
    args = parse_args()

    global VERBOSE
    VERBOSE = args.verbose

    s0_address = args.s0_addr
    s1_address = args.s1_addr
    provision_address = args.provision_addr

    hashes = get_hashes(args.public_key_files.split(','))
    generate_provision_hex_file(s0_address=s0_address,
                                s1_address=s1_address,
                                hashes=hashes,
                                provision_address=provision_address,
                                output=args.output)


if __name__ == "__main__":
    main()
