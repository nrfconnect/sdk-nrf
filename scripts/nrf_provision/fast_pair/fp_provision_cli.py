#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from intelhex import IntelHex
import argparse
import re
import sys
import base64
import math
from hashlib import sha256

_FAST_PAIR_MAGIC = (0xFA, 0x57, 0xFA, 0x57)
_ANTI_SPOOFING_KEY_LEN = 32
_SHA256_LEN = 32


def align4(address):
    return 4 * math.ceil(address / 4)


def model_id_to_bytes(model_id):
    if not re.match("^0x[0-9a-fA-F]{6}$", model_id):
        raise ValueError("Invalid Model ID")

    return bytes.fromhex(model_id.replace("0x", ""))


def anti_spoofing_key_to_bytes(anti_spoofing_key):
    anti_spoofing_key_bytes = base64.b64decode(anti_spoofing_key)

    if len(anti_spoofing_key_bytes) != _ANTI_SPOOFING_KEY_LEN:
        raise ValueError("Invalid length of Anti Spoofing key")

    return anti_spoofing_key_bytes


def prepare_fast_pair_provisioning(out_file, address, model_id, anti_spoofing_key):
    model_id_bytes = model_id_to_bytes(model_id)
    anti_spoofing_key_bytes = anti_spoofing_key_to_bytes(anti_spoofing_key)
    start_address = address
    message = sha256()

    out_hex = IntelHex()

    out_hex[address:] = _FAST_PAIR_MAGIC
    address += align4(len(_FAST_PAIR_MAGIC))

    out_hex[address:] = tuple(model_id_bytes)
    address += align4(len(model_id_bytes))

    out_hex[address:] = tuple(anti_spoofing_key_bytes)
    address += align4(len(anti_spoofing_key_bytes))

    message.update(out_hex.tobinarray(start=start_address, end=address - 1))
    assert message.digest_size == _SHA256_LEN

    out_hex[address:] = tuple(message.digest())
    address += align4(message.digest_size)

    out_hex.write_hex_file(out_file)


def parse_arguments():
    p = argparse.ArgumentParser(description='Fast Pair Provisioning Tool',
                                allow_abbrev=False)

    p.add_argument("-o", "--out_file", type=str, required=True, help="Name of the output file")
    p.add_argument("-a", "--address", type=lambda x: int(x,16), required=True,
                   help="Address of provisioning partition start (in hex)")
    p.add_argument("-m", "--model_id", type=str, required=True,
                   help="Model ID (in format 0xXXXXXX)")
    p.add_argument("-k", "--anti_spoofing_key", type=str, required=True,
                   help="Anti Spoofing Key (base64 encoded)")

    return p.parse_args()


if __name__ == "__main__":
    args = parse_arguments()

    try:
        prepare_fast_pair_provisioning(args.out_file, args.address, args.model_id,
                                       args.anti_spoofing_key)
        print("Successfully generated Fast Pair provisioning data")
    except Exception as e:
        print("Exception: {}".format(e))
        print("Failed to generate Fast Pair provisioning data")
        sys.exit(1)
