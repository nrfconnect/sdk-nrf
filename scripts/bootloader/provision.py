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


# Since cmake does not have access to DTS variables, fetch them manually.
def find_provision_memory_section(config_file):
    s0_address = 0
    s1_address = 0
    provision_address = 0

    with open(config_file, 'r') as lf:
        for line in lf.readlines():
            if "FLASH_AREA_S0_OFFSET" in line:
                s0_address = int(line.split('=')[1])
            elif "FLASH_AREA_S1_OFFSET" in line:
                s1_address = int(line.split('=')[1])
            elif "FLASH_AREA_PROVISION_OFFSET" in line:
                provision_address = int(line.split('=')[1])

    return s0_address, s1_address, provision_address


def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate provision hex file.",
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("--generated-conf-file", required=True, help="Generated conf file.")
    parser.add_argument("--public-key-files", required=True,
                        help="Semicolon-separated list of public key .pem files.")
    parser.add_argument("--signature-private-key-file", required=True,
                        help="Path to private key PEM file used to generate the signature of the image.")
    parser.add_argument("-o", "--output", required=False, default="provision.hex",
                        help="Output file name.")
    parser.add_argument("-v", "--verbose", required=False, action="count",
                        help="Verbose mode.")
    return parser.parse_args()


def get_hashes(private_signing_key_file, public_key_files):
    hashes = list()
    with open(private_signing_key_file, 'r') as fn:
        hashes.append(sha256(SigningKey.from_pem(fn.read()).get_verifying_key().to_string()).digest()[:16])
        verbose_print("hash: " + hashes[-1].hex())
    for fn in public_key_files:
        verbose_print("Getting hash of %s" % fn)
        with open(fn, 'rb') as f:
            hashes.append(sha256(VerifyingKey.from_pem(f.read()).to_string()).digest()[:16])
        verbose_print("hash: " + hashes[-1].hex())
    return hashes


def main():
    args = parse_args()

    global VERBOSE
    VERBOSE = args.verbose

    s0_address, s1_address, provision_address = find_provision_memory_section(args.generated_conf_file)
    hashes = get_hashes(args.signature_private_key_file, args.public_key_files.split(','))
    generate_provision_hex_file(s0_address=s0_address,
                                s1_address=s1_address,
                                hashes=hashes,
                                provision_address=provision_address,
                                output=args.output)


if __name__ == "__main__":
    main()
