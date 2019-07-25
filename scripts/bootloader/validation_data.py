#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic


from intelhex import IntelHex

import hashlib
import argparse
import struct
import ecdsa


def get_hash(input_hex):
    firmware_bytes = input_hex.tobinstr()
    return hashlib.sha256(firmware_bytes).digest()


def get_validation_data(signature_bytes, input_hex, public_key, magic_value):
    hash_bytes = get_hash(input_hex)
    public_key_bytes = public_key.to_string()

    # Will raise an exception if it fails
    public_key.verify(signature_bytes, hash_bytes, hashfunc=hashlib.sha256)

    validation_bytes = magic_value
    validation_bytes += struct.pack('I', input_hex.addresses()[0])
    validation_bytes += hash_bytes
    validation_bytes += public_key_bytes
    validation_bytes += signature_bytes

    return validation_bytes


def append_validation_data(signature, input_file, public_key, offset, output_file, magic_value):
    ih = IntelHex(input_file)
    ih.start_addr = None  # OBJCOPY incorrectly inserts x86 specific records, remove the start_addr as it is wrong.

    minimum_offset = ((ih.maxaddr() // 4) + 1) * 4
    if offset != 0 and offset < minimum_offset:
        raise RuntimeError("Incorrect offset, must be bigger than %x" % minimum_offset)

    # Parse comma-separated string of uint32s into hex string. Each is encoded in little-endian byte order
    parsed_magic_value = b''.join([struct.pack("<I", int(m, 0)) for m in magic_value.split(",")])

    validation_data = get_validation_data(signature_bytes=signature,
                                          input_hex=ih,
                                          public_key=public_key,
                                          magic_value=parsed_magic_value)
    validation_data_hex = IntelHex()

    # If no offset is given, append metadata right after input hex file (word aligned).
    if offset == 0:
        offset = minimum_offset

    validation_data_hex.frombytes(validation_data, offset)

    ih.merge(validation_data_hex)
    ih.write_hex_file(output_file)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Append validation metadata at specified offset.",
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-i", "--input", required=True, type=argparse.FileType('r', encoding='UTF-8'),
                        help="Input hex file.")
    parser.add_argument("--offset", required=False, type=int,
                        help="Offset to store validation metadata at.", default=0)
    parser.add_argument("-s", "--signature", required=True, type=argparse.FileType('rb'),
                        help="Signature file (DER) of ECDSA (secp256r1) signature of 'input' argument.")
    parser.add_argument("-p", "--public-key", required=True, type=argparse.FileType('r', encoding='UTF-8'),
                        help="Public key file (PEM).")
    parser.add_argument("-m", "--magic-value", required=True,
                        help="ASCII representation of magic value.")
    parser.add_argument("-o", "--output", required=False, default=None, type=argparse.FileType('w'),
                        help="Output file name. Default is to overwrite --input.")

    args = parser.parse_args()
    if args.output is None:
        args.output = args.input

    return args


def main():

    args = parse_args()

    append_validation_data(signature=args.signature.read(),
                           input_file=args.input,
                           public_key=ecdsa.VerifyingKey.from_pem(args.public_key.read()),
                           offset=args.offset,
                           output_file=args.output,
                           magic_value=args.magic_value)


if __name__ == "__main__":
    main()
