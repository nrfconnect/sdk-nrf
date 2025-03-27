#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Module for generating PSA key attribute binary blobs
"""

import argparse
import struct
import binascii
import math
import json
import sys
from pathlib import Path
from enum import IntEnum
from cryptography.hazmat.primitives import serialization

class PsaKeyType(IntEnum):
    """The type of the key"""

    AES = 0x2400
    ECC_TWISTED_EDWARDS = 0x4142
    RAW_DATA = 0x1001


class PsaKeyBits(IntEnum):
    """Number of bits in the key"""

    AES = 256
    EDDSA = 255


class PsaUsage(IntEnum):
    """Permitted usage on a key"""

    VERIFY_MESSAGE_EXPORT = 0x0801
    ENCRYPT_DECRYPT = 0x0300
    USAGE_DERIVE = 0x4000

class PsaCracenUsageSceme(IntEnum):
    NONE = 0xff
    PROTECTED = 0
    SEED = 1
    ENCRYPTED = 2
    RAW = 3

class PsaKeyLifetime(IntEnum):
    """Lifetime and location for storing key"""

    PERSISTENT_CRACEN = 0x804E0001
    PERSISTENT_CRACEN_KMU = 0x804E4B01


class PsaAlgorithm(IntEnum):
    """Algorithm that can be associated with a key. Not used for AES"""

    NONE = 0
    CBC = 0x04404000
    EDDSA_PURE = 0x06000800


class PlatformKeyAttributes:
    def __init__(self,
                 key_type: PsaKeyType,
                 identifier: int,
                 location: PsaKeyLifetime,
                 usage: PsaUsage,
                 algorithm: PsaAlgorithm,
                 size: int,
                 cracen_usage: PsaCracenUsageSceme = PsaCracenUsageSceme.NONE):

        self.key_type = key_type
        self.lifetime = location
        self.usage = usage
        self.alg0 = algorithm
        self.alg1 = PsaAlgorithm.NONE
        self.bits = size
        self.identifier = identifier

        if location == PsaKeyLifetime.PERSISTENT_CRACEN_KMU:
            if cracen_usage == PsaCracenUsageSceme.NONE:
                print("--cracen_usage must be set if location target is PERSISTENT_CRACEN_KMU")
                return
            self.identifier = 0x7fff0000 | (cracen_usage << 12) | (identifier & 0xff)

        if self.key_type == PsaKeyType.AES:
            self.alg1 = PsaAlgorithm.NONE
        elif self.key_type == PsaKeyType.ECC_TWISTED_EDWARDS:
            self.alg1 = PsaAlgorithm.NONE
        elif self.key_type == PsaKeyType.RAW_DATA:
            self.alg1 = PsaAlgorithm.NONE
        else:
            raise RuntimeError("Invalid key type")

    def pack(self):
        """Builds a binary blob compatible with the psa_key_attributes_s C struct"""

        return struct.pack(
            "<hhIIIIII",
            self.key_type,
            self.bits,
            self.lifetime,
            # Policy
            self.usage,
            self.alg0,
            self.alg1,
            self.identifier,
            0,  # Reserved, only used if key id encodes owner id
        )

def is_valid_hexa_code(string):
    try:
        int(string, 16)
        return True
    except ValueError:
        return False

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate PSA key attributes and write to stdout or"
                    "create or append the information including the key to a"
                    "nrfutil compatible json file. Also supports reading key"
                    "from a PEM file in some cases. Key source can either be"
                    "a RAW key using the --key argument, a randomly generated"
                    "key using the --trng-key argument or a public key can be"
                    "read from a .PEM file. These are mutual exclusive.",
        allow_abbrev=False,
    )

    parser.add_argument(
        "--usage",
        help="Key usage",
        type=str,
        required=True,
        choices=[x.name for x in PsaUsage],
    )

    parser.add_argument(
        "--id",
        help="Key identifier",
        type=lambda number_string: int(number_string, 0),
        required=True,
    )

    parser.add_argument(
        "--type",
        help="Key type",
        type=str,
        required=True,
        choices=[x.name for x in PsaKeyType],
    )

    parser.add_argument(
        "--size",
        help="Key size in bits",
        type=lambda number_string: int(number_string, 0),
        required=True,
    )

    parser.add_argument(
        "--algorithm",
        help="Key algorithm",
        type=str,
        required=False,
        default="NONE",
        choices=[x.name for x in PsaAlgorithm],
    )

    parser.add_argument(
        "--location",
        help="Storage location",
        type=str,
        required=True,
        choices=[x.name for x in PsaKeyLifetime],
    )

    parser.add_argument(
        "--key",
        help="Key value",
        type=str,
        required=False,
    )

    parser.add_argument(
        "--trng-key",
        help="Generate key randomly",
        action="store_true",
        required=False,
    )

    parser.add_argument(
        "--key-from-file",
        help="Read key from PEM file",
        type=argparse.FileType(mode="rb"),
        required=False,
    )

    parser.add_argument(
        "--bin-output",
        help="Output metadata as binary",
        action="store_true",
        required=False,
    )

    parser.add_argument(
        "--file",
        help="JSON file to create or modify",
        type=str,
        required=False,
    )

    parser.add_argument(
        "--cracen_usage",
        help="CRACEN KMU Slot usage scheme",
        type=str,
        required=False,
        default="NONE",
        choices=[x.name for x in PsaCracenUsageSceme],
    )

    args = parser.parse_args()

    metadata = PlatformKeyAttributes(key_type=PsaKeyType[args.type],
                              identifier=args.id,
                              location=PsaKeyLifetime[args.location],
                              usage=PsaUsage[args.usage],
                              algorithm=PsaAlgorithm[args.algorithm],
                              size=args.size,
                              cracen_usage=PsaCracenUsageSceme[args.cracen_usage]).pack()

    metadata_str = binascii.hexlify(metadata).decode('utf-8').upper()

    if args.file and Path(args.file).is_file():
        with open(args.file, encoding="utf-8") as file:
            json_data= json.load(file)
    else:
        json_data= json.loads('{ "version": 0, "keyslots": [ ]}')

    if args.trng_key:
        value  = f'TRNG:{int(math.ceil(args.size / 8))}'
    elif args.key:
        key = args.key
        while key.startswith("0x"):
            key = key.removeprefix("0x")
        if not is_valid_hexa_code(key):
            print("Invalid KEY value")
            return
        value = f'0x{key}'
    elif args.key_from_file:
        key_data = args.key_from_file.read()
        key = serialization.load_pem_public_key(key_data)
        key = key.public_bytes(
            encoding=serialization.Encoding.Raw,
            format=serialization.PublicFormat.Raw
        )
        value = f'0x{key.hex()}'
    else:
        print("Expecting either --key, --trng-key or --key-from-file")
        return

    json_data["keyslots"].append({"metadata": f'0x{metadata_str}', "value": f'{value}'})
    output = json.dumps(json_data, indent=4)

    if args.file:
        with open(args.file, "w", encoding="utf-8") as file:
            file.write(output)
    elif args.bin_output:
        sys.stdout.buffer.write(metadata)
    else:
        print(output)


if __name__ == "__main__":
    main()
