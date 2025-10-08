#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
import argparse
import os
import struct
from hashlib import sha256

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec, ed25519
from cryptography.hazmat.primitives.serialization import load_pem_public_key
from intelhex import IntelHex

# Size of implementation ID in OTP in bytes
IMPLEMENTATION_ID_SIZE = 0x20

# These variable names and values are copied from bl_storage.h and
# should be kept in sync
BL_COLLECTION_TYPE_MONOTONIC_COUNTERS = 0x1
BL_COLLECTION_TYPE_VARIABLE_DATA = 0x9312

BL_MONOTONIC_COUNTERS_DESC_NSIB = 1
BL_MONOTONIC_COUNTERS_DESC_MCUBOOT_ID0 = 2

BL_VARIABLE_DATA_TYPE_PSA_CERTIFICATION_REFERENCE = 0x1

# Maximum lengths for the TF-M attestation variable data fields.
# These are defined in tfm_attest_hal.h and in tfm_plat_device_id.h.
CERTIFICATION_REF_MAX_SIZE = 19

def generate_provision_intel_hex_file(provision_data, provision_address, output, max_size):
    assert len(provision_data) <= max_size, """Provisioning data doesn't fit.
Reduce the number of public keys, counter slots or variable data and try again."""

    ih = IntelHex()
    ih.frombytes(provision_data, offset=provision_address)
    ih.write_hex_file(output)


def add_hw_counters(provision_data, num_counter_slots_version, mcuboot_counters_slots, otp_write_width):
    num_counters = min(mcuboot_counters_slots, 1) + min(num_counter_slots_version, 1)

    if num_counters == 0:
        return provision_data

    assert num_counter_slots_version % 2 == 0, "--num-counters-slots-version must be an even number"
    assert mcuboot_counters_slots % 2 == 0, "--mcuboot-counters-slots     must be an even number"

    provision_data += struct.pack('<H', BL_COLLECTION_TYPE_MONOTONIC_COUNTERS)
    provision_data += struct.pack('<H', num_counters)  # Could be 0, 1, or 2

    if num_counter_slots_version > 0:
        provision_data += struct.pack('<H', BL_MONOTONIC_COUNTERS_DESC_NSIB)
        provision_data += struct.pack('<H', num_counter_slots_version)
        provision_data += bytes(otp_write_width * num_counter_slots_version * [0xFF])

    if mcuboot_counters_slots > 0:
        provision_data += struct.pack('<H', BL_MONOTONIC_COUNTERS_DESC_MCUBOOT_ID0)
        provision_data += struct.pack('<H', mcuboot_counters_slots)
        provision_data += bytes(otp_write_width * mcuboot_counters_slots * [0xFF])

    return provision_data


def generate_mcuboot_only_provision_hex_file(provision_address, output, max_size, mcuboot_counters_slots,
                                             lcs_state_sz, otp_write_width, variable_data):
    # This function generates a .hex file with the provisioned data
    # for the uncommon use-case where MCUBoot is present, but NSIB is
    # not.
    #
    # To simplify the FW libraries we don't make them aware of the
    # fact that NSIB is not present, so even though MCUBoot doesn't
    # use the NSIB specific metadata in
    # num_bytes_in_bl_storage_data_struct, we still provision dummy
    # data to make sure the counters are offset correctly.
    #
    # NB: An exception to the rule is num_bytes_in_num_public_keys,
    # which we set to 0 and which will be used by MCUBoot at runtime
    # to calculate where the counter are located.

    num_bytes_in_lcs = lcs_state_sz
    num_bytes_in_implementation_id = IMPLEMENTATION_ID_SIZE
    num_bytes_in_s0_address = 4
    num_bytes_in_s1_address = 4
    num_bytes_in_num_public_keys = 4

    num_bytes_in_bl_storage_data_struct = \
        num_bytes_in_lcs + \
        num_bytes_in_implementation_id + \
        num_bytes_in_s0_address + \
        num_bytes_in_s1_address + \
        num_bytes_in_num_public_keys

    provision_data = bytes(num_bytes_in_bl_storage_data_struct * [0])

    provision_data = add_hw_counters(provision_data, 0, mcuboot_counters_slots, otp_write_width)

    provision_data += variable_data

    generate_provision_intel_hex_file(provision_data, provision_address, output, max_size)


def generate_provision_hex_file(s0_address, s1_address, hashes, provision_address, output, max_size,
                                num_counter_slots_version, mcuboot_counters_slots, otp_write_width,
                                variable_data):

    provision_data = struct.pack('<III', s0_address, s1_address,
                                 len(hashes))

    idx = 0
    for mhash in hashes:
        provision_data += struct.pack('<I', 0x50FAFFFF | (idx << 24))  # Invalidation token
        provision_data += mhash
        idx += 1

    provision_data = add_hw_counters(
        provision_data,
        num_counter_slots_version,
        mcuboot_counters_slots,
        otp_write_width)

    provision_data += variable_data

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
    parser.add_argument('--mcuboot-only', required=False, action="store_true",
                        help="The MCUBOOT bootloader is used without the NSIB bootloader. Only the provision address, the MCUBOOT counters and the MCUBOOT counters slots arguments will be used.")
    parser.add_argument('--mcuboot-counters-slots', required=False, type=int, default=0,
                        help='Number of monotonic counter slots for every MCUBOOT counter.')
    parser.add_argument('--otp-write-width', required=False, type=lambda x: int(x, 0), default=2,
                        help='OTP-write width in bytes.')
    parser.add_argument('--psa-certificate-reference', required=False, type=str, default=None,
                        help='(Optional) PSA certificate reference for the TF-M attestation token.')
    return parser.parse_args()


def public_key_to_string(public_key) -> bytes:
    if isinstance(public_key, ec.EllipticCurvePublicKey):
        return public_key.public_bytes(
            encoding=serialization.Encoding.X962,
            format=serialization.PublicFormat.UncompressedPoint
        )[1:]
    if isinstance(public_key, ed25519.Ed25519PublicKey):
        return public_key.public_bytes(
            encoding=serialization.Encoding.Raw,
            format=serialization.PublicFormat.Raw
        )
    raise NotImplementedError


def get_hashes(public_key_files, verify_hashes):
    hashes = list()
    for fn in public_key_files:
        with open(fn, 'rb') as f:
            digest = sha256(public_key_to_string(load_pem_public_key(f.read()))).digest()[:16]
            if verify_hashes and any([digest[n:n + 2] == b'\xff\xff' for n in range(0, len(digest), 2)]):
                raise RuntimeError(f"Hash of key in '{os.path.abspath(f.name)}' contains 0xffff. Please regenerate the key.")
            hashes.append(digest)

    if len(hashes) != len(set(hashes)):
        raise RuntimeError('Duplicate public key found. Note that the public key corresponding to the private'
                           'key used for signing is automatically added, and must not be added explicitly.')

    return hashes

def get_variable_data(psa_certification_reference):
    # Get variable data to be written to the OTP region.
    #
    # Variable data, if present, starts with variable data collection ID, followed
    # by amount of variable data entries and the variable data entries themselves.
    # [Collection ID][Variable count][Type][Variable data length][Variable data][Type]...
    # 2 bytes        2 bytes         1 byte 1 byte                0-255 bytes
    #
    variable_data = b''
    variable_data_count = 0

    def add_variable_data(variable_data_type, data):
        nonlocal variable_data
        nonlocal variable_data_count

        variable_data += struct.pack('<B', variable_data_type)
        variable_data += struct.pack('<B', len(data))
        variable_data += data.encode('ascii')
        variable_data_count += 1

    if psa_certification_reference:
        if len(psa_certification_reference) > CERTIFICATION_REF_MAX_SIZE:
            raise RuntimeError(f"PSA certification reference is too long. Maximum length is {CERTIFICATION_REF_MAX_SIZE} characters.")
        add_variable_data(BL_VARIABLE_DATA_TYPE_PSA_CERTIFICATION_REFERENCE, psa_certification_reference)

    if variable_data_count:
        # Add the variable data header at the beginning of the variable data.
        variable_data = struct.pack('<H', BL_COLLECTION_TYPE_VARIABLE_DATA) + \
            struct.pack('<H', variable_data_count) +  \
            variable_data

        # Padding to align to 4 bytes.
        padding_length = (4 - (len(variable_data) % 4)) % 4
        variable_data += b'\x00' * padding_length

    return variable_data

def get_lcs_size(otp_write_width):

    # Life-cycle-states to be written to the OTP. The values are copied from bl_storage.h.
    lcs = {
        'BL_STORAGE_LCS_ASSEMBLY': 0x1,
        'BL_STORAGE_LCS_PROVISIONING': 0x2,
        'BL_STORAGE_LCS_SECURED': 0x3,
        'BL_STORAGE_LCS_DECOMMISSIONED': 0x4
    }

    return len(lcs) * otp_write_width

def main():
    args = parse_args()

    lcs_size = get_lcs_size(args.otp_write_width)
    num_bytes_provisioned_elsewhere = lcs_size + IMPLEMENTATION_ID_SIZE

    if not args.mcuboot_only and args.s0_addr is None:
        raise RuntimeError("Either --mcuboot-only or --s0-addr must be specified")

    variable_data = get_variable_data(
        psa_certification_reference=args.psa_certificate_reference
    )

    if args.mcuboot_only:
        generate_mcuboot_only_provision_hex_file(
            provision_address=args.provision_addr,
            output=args.output,
            max_size=args.max_size,
            mcuboot_counters_slots=args.mcuboot_counters_slots,
            lcs_state_sz=lcs_size,
            otp_write_width=args.otp_write_width,
            variable_data=variable_data
        )
        return

    s0_address = args.s0_addr
    s1_address = args.s1_addr if args.s1_addr is not None else s0_address

    # The LCS and implementation ID is stored in the OTP before the
    # rest of the provisioning data so add it to the given base
    # address
    provision_address = args.provision_addr + num_bytes_provisioned_elsewhere
    max_size = args.max_size - num_bytes_provisioned_elsewhere

    hashes = []
    if args.public_key_files:
        hashes = get_hashes(
            # Filter out empty strings
            [key for key in args.public_key_files.split(',') if key],
            not args.no_verify_hashes
        )

    generate_provision_hex_file(
        s0_address=s0_address,
        s1_address=s1_address,
        hashes=hashes,
        provision_address=provision_address,
        output=args.output,
        max_size=max_size,
        num_counter_slots_version=args.num_counter_slots_version,
        mcuboot_counters_slots=args.mcuboot_counters_slots,
        otp_write_width=args.otp_write_width,
        variable_data=variable_data
    )


if __name__ == '__main__':
    main()
