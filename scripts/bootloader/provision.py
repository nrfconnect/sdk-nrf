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

ih = IntelHex()

def add_hw_counters(nsib_counter_slots, mcuboot_counters_slots):
    num_counters = min(mcuboot_counters_slots, 1) + min(nsib_counter_slots, 1)

    if num_counters == 0:
        return bytes()

    provision_counters = struct.pack('H', 1) # Type "counter collection"
    provision_counters += struct.pack('H', num_counters)

    slots = [nsib_counter_slots, mcuboot_counters_slots]

    for i in range(2):
        if slots[i]:
            description = i + 1 # 1-indexed
            provision_counters += struct.pack('H', description)
            provision_counters += struct.pack('H', slots[i])
            provision_counters += bytes(2 * slots[i] * [0xFF])

    return provision_counters

def generate_mcuboot_only_provision_hex_file(provision_counters_address, provision_counters_size, output, mcuboot_counters_slots):
    # Add addresses
    provision_data = add_hw_counters(0, mcuboot_counters_slots)

    assert len(provision_data) <= provision_counters_size, """Provisioning data doesn't fit. Reduce the number of counter slots and try again."""

    ih.frombytes(provision_data, offset=provision_counters_address)
    ih.write_hex_file(output)

def generate_provision_hex_file(s0_address, s1_address, hashes, provision_counters_address, provision_counters_size,
                                provision_bl_storage_address, provision_bl_storage_size, output, max_size,
                                num_counter_slots_version, mcuboot_counters_slots):
    # Add addresses
    bl_storage_data = struct.pack('III', s0_address, s1_address,
                                 len(hashes))

    for mhash in hashes:
        bl_storage_data += struct.pack('I', 0xFFFFFFFF) # Invalidation token
        bl_storage_data += mhash

    assert len(bl_storage_data) <= provision_bl_storage_size, """Provisioning data doesn't fit.
Reduce the number of public keys and try again."""

    ih.frombytes(bl_storage_data, offset=provision_bl_storage_address)

    provision_counters = add_hw_counters(num_counter_slots_version, mcuboot_counters_slots)

    assert len(provision_counters) <= provision_counters_size, """Provisioning data doesn't fit.
Reduce the number of counter slots and try again."""

    ih.frombytes(provision_counters, offset=provision_counters_address)

    ih.write_hex_file(output)

def parse_args():
    parser = argparse.ArgumentParser(
        description='Generate provisioning hex file.',
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--s0-addr', type=lambda x: int(x, 0), required=False, help='Address of image slot s0')
    parser.add_argument('--s1-addr', type=lambda x: int(x, 0), required=False, help='Address of image slot s1')
    parser.add_argument('--pm-provision-counters-address', type=lambda x: int(x, 0),
                        required=True, help='PM_PROVISION_COUNTERS_ADDRESS')
    parser.add_argument('--pm-provision-counters-size', type=lambda x: int(x, 0),
                        required=True, help='PM_PROVISION_COUNTERS_SIZE')
    parser.add_argument('--pm-provision-bl-storage-address', type=lambda x: int(x, 0),
                        required=False, help='PM_PROVISION_BL_STORAGE')
    parser.add_argument('--pm-provision-bl-storage-size', type=lambda x: int(x, 0),
                        required=False, help='PM_PROVISION_BL_STORAGE_SIZE')
    parser.add_argument('--public-key-files', required=False,
                        help='Semicolon-separated list of public key .pem files.')
    parser.add_argument('-o', '--output', required=False, default='provision.hex',
                        help='Output file name.')
    parser.add_argument('--num-counter-slots-version', required=False, type=int, default=0,
                        help='Number of monotonic counter slots for version number.')
    parser.add_argument('--no-verify-hashes', required=False, action="store_true",
                        help="Don't check hashes for applicability. Use this option only for testing.")
    parser.add_argument('--mcuboot-only', required=False, action="store_true",
                        help="The MCUBOOT bootloader is used without the NSIB bootloader. Only the address, the MCUBOOT counters and the MCUBOOT counters slots arguments will be used.")
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
        generate_mcuboot_only_provision_hex_file(provision_counters_address=args.provision_counters_address,
                                                 provision_counters_size=args.provision_counters_size,
                                                 output=args.output,
                                                 mcuboot_counters_slots=args.mcuboot_counters_slots)
        return


    s0_address = args.s0_addr
    s1_address = args.s1_addr if args.s1_addr is not None else s0_address

    # The LCS and implementation ID is stored in the OTP before the
    # rest of the provisioning data so add it to the given base
    # address
    provision_bl_storage_address = args.provision_bl_storage_address + NUM_BYTES_PROVISIONED_ELSEWHERE
    provision_bl_storage_size    = args.provision_bl_storage_size    - NUM_BYTES_PROVISIONED_ELSEWHERE

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
                                provision_counters_address   =args.provision_counters_address,
                                provision_counters_size      =args.provision_counters_size,
                                provision_bl_storage_address =args.provision_bl_storage_address,
                                provision_bl_storage_size    =args.provision_bl_storage_size,
                                output=args.output,
                                num_counter_slots_version=args.num_counter_slots_version,
                                mcuboot_counters_slots=args.mcuboot_counters_slots)


if __name__ == '__main__':
    main()
