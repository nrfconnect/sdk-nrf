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
from typing import BinaryIO


class PsaKeyType(IntEnum):
    """The type of the key"""

    # PSA_KEY_TYPE_RAW_DATA
    RAW_DATA = 0x1001

    # PSA_KEY_TYPE_HMAC
    HMAC = 0x1100

    # PSA_KEY_TYPE_AES
    AES = 0x2400

    # PSA_KEY_TYPE_CHACHA20
    CHACHA20 = 0x2004

    # PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1)
    ECC_PUBLIC_KEY_SECP_R1 = 0x4112

    # PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_TWISTED_EDWARDS)
    ECC_PUBLIC_KEY_TWISTED_EDWARDS = 0x4142

    # PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1)
    ECC_KEY_PAIR_SECP_R1 = 0x7112

    # PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_TWISTED_EDWARDS)
    ECC_KEY_PAIR_TWISTED_EDWARDS = 0x7142


class PsaKeyUsage(IntEnum):
    """Permitted usage of a key"""

    # PSA_KEY_USAGE_DERIVE
    DERIVE = 0x4000

    # PSA_KEY_USAGE_ENCRYPT  | PSA_KEY_USAGE_DECRYPT
    ENCRYPT_DECRYPT = 0x0300

    # PSA_KEY_USAGE_ENCRYPT  | PSA_KEY_USAGE_DECRYPT | PSA_KEY_USAGE_EXPORT
    ENCRYPT_DECRYPT_EXPORT = 0x0301

    # PSA_KEY_USAGE_ENCRYPT  | PSA_KEY_USAGE_DECRYPT | PSA_KEY_USAGE_EXPORT | PSA_KEY_USAGE_COPY
    ENCRYPT_DECRYPT_EXPORT_COPY = 0x0303

    # PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_SIGN_HASH
    SIGN = 0x1400

    # PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_VERIFY_HASH
    SIGN_VERIFY = 0x3C00

    # PSA_KEY_USAGE_SIGN_MESSAGE | PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_EXPORT
    SIGN_VERIFY_EXPORT = 0x3C01

    # PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_VERIFY_HASH
    VERIFY = 0x2800

    # PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_EXPORT
    VERIFY_EXPORT = 0x2801

    # PSA_KEY_USAGE_VERIFY_MESSAGE | PSA_KEY_USAGE_EXPORT
    VERIFY_MESSAGE_EXPORT = 0x0801


class PsaCracenUsageScheme(IntEnum):
    """CRACEN usage scheme"""

    # The keys will be pushed to a RAM only accessible by the CRACEN
    PROTECTED = 0

    # The slots will be pushed to CRACEN’s SEED registers
    SEED = 1

    # The keys are encrypted, and are decrypted on-the-fly to a CPU-accessible RAM location before being used by the CRACEN
    ENCRYPTED = 2

    # The keys are stored as plain text and pushed to a CPU-accessible RAM location before being used by the CRACEN
    RAW = 3

    # Can only be used with PsaKeyLocation.LOCATION_CRACEN
    NONE = 0xFF


class PsaKeyPersistence(IntEnum):
    """Persistence mode for key"""

    # Default persistence option. Key slots can be reused after the key is destroyed.
    PERSISTENCE_DEFAULT = 0

    # Revoke the key slots when the key is destroyed. Key slots can not be reused.
    PERSISTENCE_REVOKABLE = 2

    # Keys are read-only or write-once, and can not be destroyed.
    PERSISTENCE_READ_ONLY = 3


class PsaKeyLocation(IntEnum):
    """Location for storing key"""

    LOCATION_CRACEN = 0x804E0000
    LOCATION_CRACEN_KMU = 0x804E4B00


class PsaAlgorithm(IntEnum):
    """Algorithm that can be associated with a key"""

    NONE = 0

    # PSA_ALG_CBC_NO_PADDING
    CBC = 0x04404000

    # PSA_ALG_CBC_PKCS7
    CBC_PKCS7 = 0x04404100

    # PSA_ALG_CTR
    CTR = 0x04C01000

    # PSA_ALG_ECB_NO_PADDING
    ECB = 0x04404400

    # PSA_ALG_CCM
    CCM = 0x05500100

    # PSA_ALG_GCM
    GCM = 0x05500200

    # PSA_ALG_CHACHA20_POLY1305
    CHAHA20_POLY1305 = 0x05100500

    # PSA_ALG_HMAC(PSA_ALG_SHA_256)
    HMAC_SHA256 = 0x03800009

    # PSA_ALG_PURE_EDDSA
    EDDSA_PURE = 0x06000800

    # PSA_ALG_ED25519PH
    ED25519PH = 0x0600090B

    # PSA_ALG_ECDSA(PSA_ALG_SHA_256)
    ECDSA_SHA256 = 0x06000609

    # PSA_ALG_ECDH
    ECDH = 0x09020000


class PlatformKeyAttributes:
    def __init__(
        self,
        key_type: PsaKeyType,
        identifier: int,
        location: PsaKeyLocation,
        key_usage: PsaKeyUsage,
        algorithm: PsaAlgorithm,
        key_bits: int,
        persistence: PsaKeyPersistence = PsaKeyPersistence.PERSISTENCE_DEFAULT,
        cracen_usage: PsaCracenUsageScheme = PsaCracenUsageScheme.RAW,
    ):
        self.key_type = key_type
        self.key_bits = key_bits
        self.key_lifetime = location | (persistence & 0xFF)
        self.usage = key_usage
        self.alg0 = algorithm
        self.alg1 = PsaAlgorithm.NONE
        self.cracen_usage = cracen_usage

        if location == PsaKeyLocation.LOCATION_CRACEN_KMU:
            if identifier < 0 or identifier > 255:
                raise RuntimeError("KMU slot number must be in range 0-255")

            if cracen_usage == PsaCracenUsageScheme.NONE:
                raise RuntimeError(
                    "--cracen-usage must be set to other than NONE if location is LOCATION_CRACEN_KMU"
                )

            self.key_id = 0x7FFF0000 | (cracen_usage << 12) | (identifier & 0xFF)
        else:
            self.key_id = identifier

    def pack(self):
        """Builds a binary blob compatible with the psa_key_attributes_s C struct"""
        return struct.pack(
            "<hhIIIIII",
            self.key_type,
            self.key_bits,
            self.key_lifetime,
            # Policy
            self.usage,
            self.alg0,
            self.alg1,
            self.key_id,
            0,  # Reserved, only used if key id encodes owner id
        )


def is_valid_hexa_code(string):
    try:
        int(string, 16)
        return True
    except ValueError:
        return False


def generate_attr_file(
    attributes: PlatformKeyAttributes,
    key_value: str | None = None,
    key_file: str | None = None,
    trng_key: bool = False,
    key_from_file: BinaryIO | None = None,
    bin_out: bool = False,
) -> None:
    metadata_str = binascii.hexlify(attributes.pack()).decode("utf-8").upper()

    if key_file and Path(key_file).is_file():
        with open(key_file, encoding="utf-8") as file:
            json_data = json.load(file)
    else:
        json_data = json.loads('{ "version": 0, "keyslots": [ ]}')

    if trng_key:
        value = f"TRNG:{int(math.ceil(attributes.key_bits / 8))}"
    elif key_value:
        key = key_value.removeprefix("0x")
        if not is_valid_hexa_code(key):
            raise RuntimeError(f"Invalid KEY value: {key}")
        value = f"0x{key}"
    elif key_from_file:
        key_data = key_from_file.read()
        try:
            public_key = serialization.load_pem_public_key(key_data)
        except ValueError:
            # it seems it is not public key, so lets try with private
            private_key = serialization.load_pem_private_key(key_data, password=None)
            public_key = private_key.public_key()
        value = f"0x{public_key.public_bytes_raw().hex()}"
    else:
        raise RuntimeError(
            "No key input received. Expecting either --key, --trng-key or --key-from-file"
        )

    json_data["keyslots"].append({"metadata": f"0x{metadata_str}", "value": f"{value}"})
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
        help="PSA key usage flags that encodes the permitted usage of a key. More details can found in the code",
        type=str,
        required=True,
        choices=[x.name for x in PsaKeyUsage],
    )

    parser.add_argument(
        "--id",
        help="Key identifier (KMU slot number (0-255) for KMU keys)",
        type=lambda number_string: int(number_string, 0),
        required=True,
    )

    parser.add_argument(
        "--type",
        help="PSA key type",
        type=str,
        required=True,
        choices=[x.name for x in PsaKeyType],
    )

    parser.add_argument(
        "--key-bits",
        help="Key size in bits. Note that ECC secp256r1 public keys must be set to 256 bits, even though the uncompressed secp256r1 public key size is 65 bytes",
        type=lambda number_string: int(number_string, 0),
        required=True,
    )

    parser.add_argument(
        "--algorithm",
        help="PSA cryptographic algorithm for the key",
        type=str,
        required=False,
        default="NONE",
        choices=[x.name for x in PsaAlgorithm],
    )

    parser.add_argument(
        "--location",
        help="PSA key storage location",
        type=str,
        required=True,
        choices=[x.name for x in PsaKeyLocation],
    )

    parser.add_argument(
        "--persistence",
        help="Persistence mode for key",
        type=str,
        required=True,
        choices=[x.name for x in PsaKeyPersistence],
    )

    parser.add_argument(
        "--cracen-usage",
        help="CRACEN KMU key usage scheme",
        type=str,
        required=False,
        default="NONE",
        choices=[x.name for x in PsaCracenUsageScheme],
    )

    parser.add_argument(
        "--key",
        help="Key value as hex-string: 0x1234567890ABCDEF... "
        "Note that ECC secp256r1 public keys should be in uncompressed format, which is 65 bytes",
        type=str,
        required=False,
    )

    parser.add_argument(
        "--trng-key",
        help="Generate key randomly on target using TRNG. Should not be done for ECC keys.",
        action="store_true",
        required=False,
    )

    parser.add_argument(
        "--key-from-file",
        help="(Experimental) Read key from PEM file",
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

    args = parser.parse_args()

    attr = PlatformKeyAttributes(
        key_type=PsaKeyType[args.type],
        identifier=args.id,
        location=PsaKeyLocation[args.location],
        key_usage=PsaKeyUsage[args.usage],
        algorithm=PsaAlgorithm[args.algorithm],
        key_bits=args.key_bits,
        persistence=PsaKeyPersistence[args.persistence],
        cracen_usage=PsaCracenUsageScheme[args.cracen_usage],
    )

    generate_attr_file(
        attributes=attr,
        key_value=args.key,
        bin_out=args.bin_output,
        trng_key=args.trng_key,
        key_from_file=args.key_from_file,
        key_file=args.file,
    )
