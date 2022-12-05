#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause


from intelhex import IntelHex

import argparse
import struct
from ecdsa import VerifyingKey
from hashlib import sha256
import os

# Size of LCS storage and implementation ID in OTP in bytes
LCS_STATE_SIZE = 0x8
IMPLEMENTATION_ID_SIZE = 0x20
NUM_BYTES_PROVISIONED_ELSEWHERE = LCS_STATE_SIZE + IMPLEMENTATION_ID_SIZE

def main():
    args = parse_args()

    s0_address = args.s0_addr
    s1_address = args.s1_addr if args.s1_addr is not None else s0_address

    # The LCS and implementation ID is stored in the OTP before the
    # rest of the provisioning data so add it to the given base
    # address
    provision_bl_storage_address = args.address + NUM_BYTES_PROVISIONED_ELSEWHERE
    provision_bl_storage_size    = args.size    - NUM_BYTES_PROVISIONED_ELSEWHERE

    hashes = []
    if args.public_key_files:
        hashes = get_hashes(
            # Filter out empty strings
            [key for key in args.public_key_files.split(',') if key]
        )

    # bl_storage does not support keys with 0xFFFF
    if not args.no_verify_hashes:
        for mhash in hashes:
            if any([mhash[n:n+2] == b'\xff\xff' for n in range(0, len(mhash), 2)]):
                raise RuntimeError("Hash of a public key contains 0xffff. Please regenerate the key.")

    # Duplicate keys are not supported as they indicate user-error
    if len(hashes) != len(set(hashes)):
        raise RuntimeError('Duplicate public key found. Note that the public key corresponding to the private'
                           'key used for signing is automatically added, and must not be added explicitly.')

    # Add addresses
    bl_storage_data = struct.pack('III', s0_address, s1_address,
                                 len(hashes))

    for mhash in hashes:
        bl_storage_data += struct.pack('I', 0xFFFFFFFF) # Invalidation token
        bl_storage_data += mhash

    assert len(bl_storage_data) <= provision_bl_storage_size, """Provisioning data doesn't fit.
Reduce the number of public keys and try again."""

    ih = IntelHex()

    ih.frombytes(bl_storage_data, offset=provision_bl_storage_address)

    ih.write_hex_file(args.output)

def parse_args():
    parser = argparse.ArgumentParser(
        description='Generate provisioning hex file.',
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--s0-addr', type=lambda x: int(x, 0), required=True, help='Address of image slot s0')
    parser.add_argument('--s1-addr', type=lambda x: int(x, 0), required=False, help='Address of image slot s1')
    parser.add_argument('--address', type=lambda x: int(x, 0),
                        required=True, help='PM_PROVISION_BL_STORAGE')
    parser.add_argument('--size', type=lambda x: int(x, 0),
                        required=True, help='PM_PROVISION_BL_STORAGE_SIZE')
    parser.add_argument('--public-key-files', required=False,
                        help='Semicolon-separated list of public key .pem files.')
    parser.add_argument('-o', '--output', required=False, default='provision.hex',
                        help='Output file name.')
    parser.add_argument('--no-verify-hashes', required=False, action="store_true",
                        help="Don't check hashes for applicability. Use this option only for testing.")
    return parser.parse_args()


def get_hashes(public_key_files):
    hashes = []
    for fn in public_key_files:
        with open(fn, 'rb') as f:
            public_key_as_string = VerifyingKey.from_pem(f.read()).to_string()
            sha256_of_public_key = sha256(public_key_as_string).digest()
            hashes.append(sha256_of_public_key[:16])
    return hashes



if __name__ == '__main__':
    main()
