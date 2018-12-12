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

VERBOSE = False


def verbose_print(*printstrs):
    if VERBOSE:
        print(*printstrs)


def get_hash(input_hex):
    firmware_bytes = input_hex.tobinstr()
    verbose_print("firmware len: %d (0x%x)" % (len(firmware_bytes), len(firmware_bytes)))
    return hashlib.sha256(firmware_bytes).digest()


def get_validation_data(signature_bytes, input_hex, public_key, magic_value, pk_hash_len):
    hash_bytes = get_hash(input_hex)
    public_key_bytes = public_key.to_string()
    public_key_hash_bytes = hashlib.sha256(public_key_bytes).digest()[:pk_hash_len]

    verbose_print("magic value (hex) (len: %d): %s" % (len(magic_value), bytes.hex(magic_value)))
    verbose_print("firmware hash (hex) (len: %d): %s" % (len(hash_bytes), bytes.hex(hash_bytes)))
    verbose_print("public key (hex) (len: %d): %s" % (len(public_key_bytes), bytes.hex(public_key_bytes)))
    verbose_print("truncated public key hash (hex) (len: %d): %s" % (len(public_key_hash_bytes), bytes.hex(public_key_hash_bytes)))
    verbose_print("signature (hex) (len: %d): %s" % (len(signature_bytes), bytes.hex(signature_bytes)))

    verbose_print("signature ok? ", public_key.verify(signature_bytes, hash_bytes, hashfunc=hashlib.sha256))

    validation_bytes = magic_value
    validation_bytes += struct.pack('I', input_hex.addresses()[0])
    validation_bytes += hash_bytes
    validation_bytes += public_key_bytes
    validation_bytes += signature_bytes

    return validation_bytes


def sign_and_append_validation_data(signature, input_file, public_key, offset, output_file, magic_value, pk_hash_len):
    ih = IntelHex(input_file)
    ih.start_addr = None  # OBJCOPY incorrectly inserts x86 specific records, remove the start_addr as it is wrong.

    minimum_offset = ((ih.maxaddr() // 4) + 1) * 4
    if offset != 0 and offset < minimum_offset:
        raise RuntimeError("Incorrect offset, must be bigger than %x" % minimum_offset)

    # Parse comma-separated string of uint32s into hex string. Each in is encoded
    # in little-endian byte order
    parsed_magic_value = b''.join([struct.pack("<I", int(m, 0)) for m in magic_value.split(",")])

    validation_data = get_validation_data(signature_bytes=signature,
                                          input_hex=ih,
                                          public_key=public_key,
                                          magic_value=parsed_magic_value,
                                          pk_hash_len=pk_hash_len)
    validation_data_hex = IntelHex()

    # If no offset is given, append metadata right after input hex file (word aligned).
    if offset == 0:
        offset = minimum_offset

    validation_data_hex.frombytes(validation_data, offset)

    ih.merge(validation_data_hex)
    ih.write_hex_file(output_file)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Sign hex file and append metadata at specified offset.",
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-i", "--input", required=True, type=argparse.FileType('r', encoding='UTF-8'),
                        help="Hex file to sign.")
    parser.add_argument("-o", "--offset", required=False, type=int,
                        help="Offset to store validation metadata at.", default=0)
    parser.add_argument("-s", "--signature", required=True, type=argparse.FileType('rb'),
                        help="Signature file (DER).")
    parser.add_argument("-p", "--public-key", required=True, type=argparse.FileType('r', encoding='UTF-8'),
                        help="Public key file (PEM).")
    parser.add_argument("-m", "--magic-value", required=True,
                        help="ASCII representation of magic value.")
    parser.add_argument("-l", "--pk-hash-len", required=False, type=int, default=-1,
                        help="The length to truncate the public key hash to. Default: no truncation.")
    parser.add_argument("--output", required=False, default=None, type=argparse.FileType('w'),
                        help="Output file name Default is to overwrite --input.")
    parser.add_argument("-v", "--verbose", required=False, action="count",
                        help="Verbose mode.")

    args = parser.parse_args()
    if args.output is None:
        args.output = args.input

    return args


def main():

    args = parse_args()

    global VERBOSE
    VERBOSE = args.verbose

    sign_and_append_validation_data(signature=args.signature.read(),
                                    input_file=args.input,
                                    public_key=ecdsa.VerifyingKey.from_pem(args.public_key.read()),
                                    offset=args.offset,
                                    output_file=args.output,
                                    magic_value=args.magic_value,
                                    pk_hash_len=args.pk_hash_len)


if __name__ == "__main__":
    main()
