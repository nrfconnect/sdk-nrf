#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from __future__ import annotations

import abc
import argparse
import hashlib
import struct
import sys
from pathlib import Path
from typing import BinaryIO, TextIO

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, ed25519
from cryptography.hazmat.primitives.asymmetric.utils import encode_dss_signature
from cryptography.hazmat.primitives.serialization import load_pem_public_key
from intelhex import IntelHex  # type: ignore[import-untyped]


def load_public_key(file_name: Path):
    with open(file_name, 'rb') as f:
        return load_pem_public_key(f.read())


class InvalidSignatureError(Exception):
    """Raised when an invalid signature is encountered."""


class BaseValidator(abc.ABC):

    def __init__(self, hashfunc=None) -> None:
        """
        param hashfunc: hashing function e.g. hashlib.sha256
        """
        self.hashfunc = hashfunc

    def get_hash(self, input_hex: IntelHex) -> bytes:
        firmware_bytes = input_hex.tobinstr()
        return self.hashfunc(firmware_bytes).digest()

    @abc.abstractmethod
    def to_string(self, public_key) -> bytes:
        """Serialize the public key to bytes."""

    @abc.abstractmethod
    def verify(self, public_key, signature: bytes, message_bytes: bytes):
        """Verify signature. Raise an exception if not valid."""

    def get_validation_data(
        self,
        signature_bytes: bytes,
        input_hex: IntelHex,
        public_key,
        magic_value: bytes
    ) -> bytes:
        hash_bytes = self.get_hash(input_hex)
        public_key_bytes = self.to_string(public_key)

        # Will raise an exception if it fails
        self.verify(public_key, signature_bytes, hash_bytes)

        validation_bytes = magic_value
        validation_bytes += struct.pack('<I', input_hex.addresses()[0])
        validation_bytes += hash_bytes
        validation_bytes += public_key_bytes
        validation_bytes += signature_bytes

        return validation_bytes

    def append_validation_data(
        self,
        signature_file: Path,
        input_file: Path,
        public_key: ec.EllipticCurvePublicKey | ed25519.Ed25519PublicKey,
        offset: int,
        output_hex: TextIO,
        output_bin: BinaryIO | None,
        magic_value: str
    ) -> None:
        with open(input_file, encoding='UTF-8') as f:
            ih = IntelHex(f)
        # OBJCOPY incorrectly inserts x86 specific records, remove the start_addr as it is wrong.
        ih.start_addr = None

        minimum_offset = ((ih.maxaddr() // 4) + 1) * 4
        if offset != 0 and offset < minimum_offset:
            raise RuntimeError(f'Incorrect offset, must be bigger than {hex(minimum_offset)}')

        # Parse comma-separated string of uint32s into hex string. Each is encoded in little-endian byte order
        parsed_magic_value = self._parse_magic_value(magic_value)
        signature_bytes = self._read_binary(signature_file)
        validation_data = self.get_validation_data(
            signature_bytes=signature_bytes,
            input_hex=ih,
            public_key=public_key,
            magic_value=parsed_magic_value
        )
        validation_data_hex = IntelHex()

        # If no offset is given, append metadata right after input hex file (word aligned).
        if offset == 0:
            offset = minimum_offset

        validation_data_hex.frombytes(validation_data, offset)

        ih.merge(validation_data_hex)
        ih.write_hex_file(output_hex)

        if output_bin:
            ih.tofile(output_bin.name, format='bin')

    @staticmethod
    def _parse_magic_value(magic_value):
        parsed_magic_value = b''.join(
            [struct.pack('<I', int(m, 0)) for m in magic_value.split(',')]
        )
        return parsed_magic_value

    @staticmethod
    def _read_binary(file, encoding=None) -> bytes:
        with open(file, 'rb', encoding=encoding) as f:
            data_bytes = f.read()
        return data_bytes

    @staticmethod
    def _read_text(file, encoding=None) -> str:
        with open(file, encoding=encoding) as f:
            data = f.read()
        return data


class EcdsaSignatureValidator(BaseValidator):

    def get_validation_data(
        self,
        signature_bytes: bytes,
        input_hex: IntelHex,
        public_key,
        magic_value: bytes
    ) -> bytes:
        hash_bytes = self.get_hash(input_hex)
        public_key_bytes = self.to_string(public_key)

        # Will raise an exception if it fails
        self.verify(public_key, signature_bytes, hash_bytes)

        validation_bytes = magic_value
        validation_bytes += struct.pack('<I', input_hex.addresses()[0])
        validation_bytes += hash_bytes
        validation_bytes += public_key_bytes
        validation_bytes += signature_bytes

        return validation_bytes

    def to_string(self, public_key: ec.EllipticCurvePublicKey) -> bytes:
        public_key_bytes = public_key.public_bytes(
            encoding=serialization.Encoding.X962,
            format=serialization.PublicFormat.UncompressedPoint,
        )
        return public_key_bytes[1:]

    def verify(
        self, public_key: ec.EllipticCurvePublicKey, signature_bytes: bytes, message_bytes: bytes
    ) -> None:
        signature = self._encode_signature(signature_bytes)
        public_key.verify(signature, message_bytes, ec.ECDSA(hashes.SHA256()))

    @staticmethod
    def _encode_signature(signature: bytes) -> bytes:
        """Return encoded signature from ASN.1 format signature."""
        assert len(signature) == 64
        values = signature[:32], signature[32:]
        key1, key2 = (int.from_bytes(v, byteorder='big') for v in values)
        return encode_dss_signature(key1, key2)


class Ed25519SignatureValidator(BaseValidator):

    def to_string(self, public_key) -> bytes:
        """Serialize the public key to bytes."""
        public_key_bytes = public_key.public_bytes(
            encoding=serialization.Encoding.Raw,
            format=serialization.PublicFormat.Raw
        )
        return public_key_bytes

    def verify(
        self,
        public_key: ed25519.Ed25519PublicKey,
        signature_bytes: bytes,
        message_bytes: bytes
    ):
        try:
            public_key.verify(signature_bytes, message_bytes)
        except InvalidSignature:
            raise InvalidSignatureError('Signature verification failed')

    def get_validation_data(
        self,
        signature_bytes: bytes,
        input_hex: IntelHex,
        public_key: ed25519.Ed25519PublicKey,
        magic_value: bytes
    ) -> bytes:
        if self.hashfunc:
            return super().get_validation_data(
                signature_bytes=signature_bytes,
                input_hex=input_hex,
                public_key=public_key,
                magic_value=magic_value
            )
        validation_bytes = magic_value
        validation_bytes += struct.pack('<I', input_hex.addresses()[0])
        validation_bytes += signature_bytes

        return validation_bytes

    def append_validation_data(
        self,
        signature_file: Path,
        input_file: Path,
        public_key: ec.EllipticCurvePublicKey | ed25519.Ed25519PublicKey,
        offset: int,
        output_hex: TextIO,
        output_bin: BinaryIO | None,
        magic_value: str
    ) -> None:
        if self.hashfunc:
            return super().append_validation_data(
                signature_file=signature_file,
                input_file=input_file,
                public_key=public_key,
                offset=offset,
                output_hex=output_hex,
                output_bin=output_bin,
                magic_value=magic_value
            )

        with open(input_file, encoding='UTF-8') as f:
            ih = IntelHex(f)
        # OBJCOPY incorrectly inserts x86 specific records, remove the start_addr as it is wrong.
        ih.start_addr = None

        minimum_offset = ((ih.maxaddr() // 4) + 1) * 4
        if offset != 0 and offset < minimum_offset:
            raise RuntimeError(f'Incorrect offset, must be bigger than {hex(minimum_offset)}')

        # Parse comma-separated string of uint32s into hex string. Each is encoded in little-endian byte order
        parsed_magic_value = self._parse_magic_value(magic_value)
        signature_bytes = self._read_binary(signature_file)
        self.verify(
            public_key=public_key,
            signature_bytes=signature_bytes,
            message_bytes=ih.tobinstr()
        )
        validation_data = self.get_validation_data(
            signature_bytes=signature_bytes,
            input_hex=ih,
            public_key=public_key,
            magic_value=parsed_magic_value
        )
        validation_data_hex = IntelHex()

        # If no offset is given, append metadata right after input hex file (word aligned).
        if offset == 0:
            offset = minimum_offset

        validation_data_hex.frombytes(validation_data, offset)

        ih.merge(validation_data_hex)
        ih.write_hex_file(output_hex)

        if output_bin:
            ih.tofile(output_bin.name, format='bin')


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description='Append validation metadata at specified offset. Generate HEX and BIN file',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False
    )

    parser.add_argument(
        '--algorithm', '-a', dest='algorithm', help='Signing algorithm (default: %(default)s)',
        choices=['ecdsa', 'ed25519'], default='ecdsa'
    )

    parser.add_argument('-i', '--input', required=True,
                        type=Path, help='Input hex file.')
    parser.add_argument('--offset', required=False, type=int,
                        help='Offset to store validation metadata at.', default=0)
    parser.add_argument('-s', '--signature', required=True, type=Path,
                        help="Signature file (DER) of ECDSA (secp256r1) or ED25519 signature of 'input' argument.")
    parser.add_argument('-p', '--public-key', required=True,
                        type=Path, help='Public key file (PEM).')
    parser.add_argument('-m', '--magic-value', required=True,
                        help='ASCII representation of magic value.')
    parser.add_argument('-o', '--output-hex', required=False, default=None,
                        type=argparse.FileType('w'),
                        help='.hex output file name. Default is to overwrite --input.')
    parser.add_argument('--output-bin', required=False, default=None,
                        type=argparse.FileType('w'),
                        help='.bin output file name.')
    parser.add_argument('--hash', default=None, choices=['sha512'],
                        help='Hash algorithm to use with ed25519.')

    args = parser.parse_args(argv)
    if args.output_hex is None:
        args.output_hex = args.input

    return args


def main(argv=None) -> int:
    args = parse_args(argv)

    signature_validator: EcdsaSignatureValidator | Ed25519SignatureValidator
    if args.algorithm == 'ecdsa':
        signature_validator = EcdsaSignatureValidator(hashfunc=hashlib.sha256)
    elif args.algorithm == 'ed25519':
        hashfunction = hashlib.sha512 if args.hash == 'sha512' else None
        signature_validator = Ed25519SignatureValidator(hashfunction)
    else:
        raise SystemExit('Algorithm not implemented')

    public_key = load_public_key(args.public_key)
    signature_validator.append_validation_data(
        signature_file=args.signature,
        input_file=args.input,
        public_key=public_key,
        offset=args.offset,
        output_hex=args.output_hex,
        output_bin=args.output_bin,
        magic_value=args.magic_value
    )
    return 0


if __name__ == '__main__':
    sys.exit(main())
