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

def generate_provision_intel_hex_file(provision_data, provision_address, output, max_size):
    assert len(provision_data) <= max_size, """Provisioning data doesn't fit.
Reduce the number of public keys or counter slots and try again."""

    ih = IntelHex()
    ih.frombytes(provision_data, offset=provision_address)
    ih.write_hex_file(output)

def add_hw_counters(provision_data, num_counter_slots_version):
    num_counters = min(num_counter_slots_version, 1)

    if num_counters == 0:
        return provision_data

    assert num_counter_slots_version % 2 == 0, "--num-counters-slots-version must be an even number"

    provision_data += struct.pack('H', 1) # Type "counter collection"
    provision_data += struct.pack('H', num_counters)

    if num_counter_slots_version > 0:
        provision_data += struct.pack('H', 1) # counter description
        provision_data += struct.pack('H', num_counter_slots_version)
        provision_data += bytes(2 * num_counter_slots_version * [0xFF])

    return provision_data

def generate_provision_hex_file(s0_address, s1_address, hashes, provision_address, output, max_size,
                                num_counter_slots_version):
    provision_data = struct.pack('III', s0_address, s1_address,
                                 len(hashes))

    for mhash in hashes:
        provision_data += struct.pack('I', 0xFFFFFFFF) # Invalidation token
        provision_data += mhash

    provision_data = add_hw_counters(provision_data, num_counter_slots_version)

    generate_provision_intel_hex_file(provision_data, provision_address, output, max_size)


def parse_args():
    parser = argparse.ArgumentParser(
        description='Generate provisioning hex file.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False)
    parser.add_argument('--s0-addr', type=lambda x: int(x, 0), required=False, help='Address of image slot s0')
    parser.add_argument('--s1-addr', type=lambda x: int(x, 0), required=False, help='Address of image slot s1')
    parser.add_argument('--provision-addr', type=lambda x: int(x, 0),
                        required=True, help='Address at which to place the provisioned data')
    parser.add_argument('--public-key-files', required=False,
                        help='Semicolon-separated list of public key .pem files.')
    parser.add_argument('-o', '--output', required=False, default='provision.hex',
                        help='Output file name.')
    parser.add_argument('--max-size', required=False, type=lambda x: int(x, 0), default=0x1000,
                        help='Maximum total size of the provision data, including the counter slots.')
    parser.add_argument('--num-counter-slots-version', required=False, type=int, default=0,
                        help='Number of monotonic counter slots for version number.')
    parser.add_argument('--no-verify-hashes', required=False, action="store_true",
                        help="Don't check hashes for applicability. Use this option only for testing.")
    return parser.parse_args()


def get_hashes(public_key_files, verify_hashes):
    hashes = list()
    for fn in public_key_files:
        with open(fn, 'rb') as f:
            digest = sha256(VerifyingKey.from_pem(f.read()).to_string()).digest()[:16]
            if verify_hashes and any([digest[n:n+2] == b'\xff\xff' for n in range(0, len(digest), 2)]):
                raise RuntimeError("Hash of key in '%s' contains 0xffff. Please regenerate the key." %
                                   os.path.abspath(f.name))
            hashes.append(digest)

    if len(hashes) != len(set(hashes)):
        raise RuntimeError('Duplicate public key found. Note that the public key corresponding to the private'
                           'key used for signing is automatically added, and must not be added explicitly.')

    return hashes


def main():
    args = parse_args()

    s0_address = args.s0_addr
    s1_address = args.s1_addr if args.s1_addr is not None else s0_address
    # The LCS and implementation ID is stored in the OTP before the
    # rest of the provisioning data so add it to the given base
    # address
    provision_address = args.provision_addr + NUM_BYTES_PROVISIONED_ELSEWHERE
    max_size          = args.max_size       - NUM_BYTES_PROVISIONED_ELSEWHERE

    hashes = []
    if args.public_key_files:
        hashes = get_hashes(
            # Filter out empty strings
            [key for key in args.public_key_files.split(',') if key],
            not args.no_verify_hashes
        )

    generate_provision_hex_file(s0_address=s0_address,
                                s1_address=s1_address,
                                hashes=hashes,
                                provision_address=provision_address,
                                output=args.output,
                                max_size=max_size,
                                num_counter_slots_version=args.num_counter_slots_version)


if __name__ == '__main__':
    main()
