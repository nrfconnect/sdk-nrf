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


def generate_provision_hex_file(s0_address, s1_address, hashes, provision_address, output, max_size,
                                num_counter_slots_version):
    # Add addresses
    provision_data = struct.pack('III', s0_address, s1_address, len(hashes))
    for mhash in hashes:
        provision_data += struct.pack('I', 0xFFFFFFFF) # Invalidation token
        provision_data += mhash

    num_counters = 1 if num_counter_slots_version > 0 else 0
    provision_data += struct.pack('H', 1) # Type "counter collection"
    provision_data += struct.pack('H', num_counters)

    if num_counters == 1:
        if num_counter_slots_version % 2 == 1:
            num_counter_slots_version += 1
            print(f'Monotonic counter slots rounded up to {num_counter_slots_version}')
        provision_data += struct.pack('H', 1) # counter description
        provision_data += struct.pack('H', num_counter_slots_version)

    assert (len(provision_data) + (2 * num_counter_slots_version)) <= max_size, """Provisioning data doesn't fit.
Reduce the number of public keys or counter slots and try again."""

    ih = IntelHex()
    ih.frombytes(provision_data, offset=provision_address)
    ih.write_hex_file(output)


def parse_args():
    parser = argparse.ArgumentParser(
        description='Generate provisioning hex file.',
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--s0-addr', type=lambda x: int(x, 0), required=True, help='Address of image slot s0')
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
    return parser.parse_args()


def get_hashes(public_key_files):
    hashes = list()
    for fn in public_key_files:
        with open(fn, 'rb') as f:
            digest = sha256(VerifyingKey.from_pem(f.read()).to_string()).digest()[:16]
            if any([digest[n:n+2] == b'\xff\xff' for n in range(0, len(digest), 2)]):
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
    provision_address = args.provision_addr

    hashes = get_hashes(args.public_key_files.split(',')) if args.public_key_files else list()
    generate_provision_hex_file(s0_address=s0_address,
                                s1_address=s1_address,
                                hashes=hashes,
                                provision_address=provision_address,
                                output=args.output,
                                max_size=args.max_size,
                                num_counter_slots_version=args.num_counter_slots_version)


if __name__ == '__main__':
    main()
