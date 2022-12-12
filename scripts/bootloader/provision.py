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

def generate_provision_intel_hex_file(provision_data, prov_offset, output_file):
    ih = IntelHex()
    ih.frombytes(provision_data, offset=prov_offset)
    ih.write_hex_file(output_file)

def add_hw_counters(provision_data, nsib_counter_slots, mcuboot_counters_num, mcuboot_counters_slots):
    all_counters_num = mcuboot_counters_num + min(nsib_counter_slots, 1)

    if all_counters_num == 0:
        return provision_data

    # The number of slots is rounded up to the nearest even number to ensure that the total size
    # of header and slots is aligned on a 32-bit word.
    if nsib_counter_slots > 0 and (nsib_counter_slots % 2 == 1):
            nsib_counter_slots+= 1
            print(f'Monotonic counter slots for NSIB rounded up to {nsib_counter_slots}')

    if mcuboot_counters_slots > 0 and (mcuboot_counters_slots % 2 == 1):
            mcuboot_counters_slots+= 1
            print(f'Monotonic counter slots for MCUBOOT rounded up to {mcuboot_counters_slots}')

    provision_data += struct.pack('H', 1) # Type "counter collection"
    provision_data += struct.pack('H', all_counters_num)

    if nsib_counter_slots > 0:
            provision_data += struct.pack('H', 1) # counter description
            provision_data += struct.pack('H', nsib_counter_slots)
            provision_data += bytes(2 * nsib_counter_slots * [0xFF])

    for i in range(0, mcuboot_counters_num):
        provision_data += struct.pack('H', i + 2) # MCUBOOT counters ID start from 2
        provision_data += struct.pack('H', mcuboot_counters_slots)
        provision_data += bytes(2 * mcuboot_counters_slots* [0xFF])

    return provision_data

def generate_mcuboot_only_provision_hex_file(provision_address, output, max_size, mcuboot_counters_num, mcuboot_counters_slots):
    # Add addresses
    provision_data = add_hw_counters(bytes(), 0, mcuboot_counters_num, mcuboot_counters_slots)

    assert len(provision_data) <= max_size, """Provisioning data doesn't fit. Reduce the number of counter slots and try again."""

    generate_provision_intel_hex_file(provision_data, provision_address, output)

def generate_provision_hex_file(s0_address, s1_address, hashes, provision_address, output, max_size,
                                num_counter_slots_version, mcuboot_counters_num, mcuboot_counters_slots):
    # Add addresses
    provision_data = struct.pack('III', s0_address, s1_address,
                                 len(hashes))

    for mhash in hashes:
        provision_data += struct.pack('I', 0xFFFFFFFF) # Invalidation token
        provision_data += mhash

    provision_data = add_hw_counters(provision_data, num_counter_slots_version, mcuboot_counters_num, mcuboot_counters_slots)

    assert len(provision_data) <= max_size, """Provisioning data doesn't fit.
Reduce the number of public keys or counter slots and try again."""

    generate_provision_intel_hex_file(provision_data, provision_address, output)

def parse_args():
    parser = argparse.ArgumentParser(
        description='Generate provisioning hex file.',
        formatter_class=argparse.RawDescriptionHelpFormatter)
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
    parser.add_argument('--mcuboot-only', required=False, action="store_true",
                        help="The MCUBOOT bootloader is used without the NSIB bootloader. Only the provision address, the MCUBOOT counters and the MCUBOOT counters slots arguments will be used.")
    parser.add_argument('--mcuboot-counters-num', required=False, type=int,  choices=range(0, 4), default=0,
                        help='Number of monotonic counters used by MCUBOOT.')
    parser.add_argument('--mcuboot-counters-slots', required=False, type=int, default=0,
                        help='Number of monotonic counter slots for every MCUBOOT counter.')
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

    if not args.mcuboot_only and args.s0_addr is None:
        raise RuntimeError("--s0-addr parameter is required when --mcuboot-only is not enabled.")

    if args.mcuboot_only:
        generate_mcuboot_only_provision_hex_file(provision_address=args.provision_addr,
                                                 output=args.output,
                                                 max_size=args.max_size,
                                                 mcuboot_counters_num=args.mcuboot_counters_num,
                                                 mcuboot_counters_slots=args.mcuboot_counters_slots)
        return


    s0_address = args.s0_addr
    s1_address = args.s1_addr if args.s1_addr is not None else s0_address

    # The LCS is stored in the OTP before the rest of the provisioning
    # data so add it to the given base address
    provision_address = args.provision_addr + LCS_STATE_SIZE + IMPLEMENTATION_ID_SIZE

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
                                max_size=args.max_size,
                                num_counter_slots_version=args.num_counter_slots_version,
                                mcuboot_counters_num=args.mcuboot_counters_num,
                                mcuboot_counters_slots=args.mcuboot_counters_slots)


if __name__ == '__main__':
    main()
