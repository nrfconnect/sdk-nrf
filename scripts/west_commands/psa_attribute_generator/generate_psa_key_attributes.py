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
    ENCRYPT_DECRYPT_EXPORT_COPY = 0x0303
    ENCRYPT_DECRYPT_EXPORT = 0x0301
    SIGN_VERIFY_EXPORT = 0x3C01

class PsaCracenUsageSceme(IntEnum):
    NONE = 0xff
    PROTECTED = 0
    SEED = 1
    ENCRYPTED = 2
    RAW = 3

class PsaKeyLifetime(IntEnum):
    """ Lifetime for key """

    PERSISTENCE_VOLATILE = 0x00
    PERSISTENCE_DEFAULT = 0x01
    PERSISTENCE_REVOKABLE = 0x02
    PERSISTENCE_READ_ONLY = 0x03

class PsaKeyLocation(IntEnum):
    """Location for storing key"""

    LOCATION_CRACEN = 0x804E0000
    LOCATION_CRACEN_KMU = 0x804E4B00


class PsaAlgorithm(IntEnum):
    """Algorithm that can be associated with a key. Not used for AES"""

    NONE = 0
    CBC = 0x04404000
    EDDSA_PURE = 0x06000800


class PlatformKeyAttributes:
    def __init__(self,
                 key_type: PsaKeyType,
                 identifier: int,
                 location: PsaKeyLocation,
                 lifetime: PsaKeyLifetime,
                 usage: PsaUsage,
                 algorithm: PsaAlgorithm,
                 size: int,
                 cracen_usage: PsaCracenUsageSceme = PsaCracenUsageSceme.NONE):

        self.key_type = key_type
        self.location = location
        self.lifetime = lifetime
        self.usage = usage
        self.alg0 = algorithm
        self.alg1 = PsaAlgorithm.NONE
        self.size = size
        self.identifier = identifier

        if location == PsaKeyLocation.LOCATION_CRACEN_KMU:
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

        location_lifetime = self.location | self.lifetime

        return struct.pack(
            "<hhIIIIII",
            self.key_type,
            self.size,
            location_lifetime,
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

def generate_attr_file(attributes: PlatformKeyAttributes, key_value: str, key_file: str, trng_key: bool, key_from_file: str, bin_out: bool) -> None:

    metadata_str = binascii.hexlify(attributes.pack()).decode('utf-8').upper()

    if key_file and Path(key_file).is_file():
        with open(key_file, encoding="utf-8") as file:
            json_data= json.load(file)
    else:
        json_data= json.loads('{ "version": 0, "keyslots": [ ]}')

    if trng_key:
        value  = f'TRNG:{int(math.ceil(attributes.size / 8))}'
    elif key_value:
        key = key_value.strip("0x")
        if not is_valid_hexa_code(key):
            print("Invalid KEY value")
            return
        value = f'0x{key}'
    elif key_from_file != "":
        with open(key_from_file, "rb") as f:
            key_data = f.read()
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

    if key_file is not None:
        with open(key_file, "w", encoding="utf-8") as file:
            file.write(output)
    elif bin_out:
        sys.stdout.buffer.write(attributes.pack())
    else:
        print(output)


if __name__ == "__main__":
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
        choices=[x.name for x in PsaKeyLocation],
    )

    parser.add_argument(
        "--lifetime",
        help="Persistence level",
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
        type=str,
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

    attr = PlatformKeyAttributes(key_type=PsaKeyType[args.type],
                     identifier=args.id,
                     location=PsaKeyLocation[args.location],
                     lifetime=PsaKeyLifetime[args.lifetime],
                     usage=PsaUsage[args.usage],
                     algorithm=PsaAlgorithm[args.algorithm],
                     size=args.size,
                     cracen_usage=PsaCracenUsageSceme[args.cracen_usage])

    generate_attr_file(attributes=attr,
         key_value=args.key,
         bin_out=args.bin_output,
         trng_key=args.trng_key,
         key_from_file=args.key_from_file,
         key_file=args.file)
