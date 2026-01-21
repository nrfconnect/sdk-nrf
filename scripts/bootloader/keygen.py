#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from __future__ import annotations

import abc
import argparse
import sys
from hashlib import sha256, sha512
from typing import BinaryIO

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, ed25519
from cryptography.hazmat.primitives.serialization import load_pem_private_key


def generate_legal_key_for_elliptic_curve():
    """
    Ensure that we don't have 0xFFFF in the hash of the public key of
    the generated keypair.

    :return: A key who's SHA256 digest does not contain 0xFFFF
    """
    while True:
        key = ec.generate_private_key(ec.SECP256R1())
        public_bytes = key.public_key().public_bytes(
            encoding=serialization.Encoding.X962,
            format=serialization.PublicFormat.UncompressedPoint,
        )

        # The digest don't contain the first byte as it denotes
        # if it is compressed/UncompressedPoint.
        digest = sha256(public_bytes[1:]).digest()[:16]
        if not (any([digest[n:n + 2] == b'\xff\xff'
                     for n in range(0, len(digest), 2)])):
            return key


def generate_legal_key_for_ed25519():
    """
    Ensure that we don't have 0xFFFF in the hash of the public key of
    the generated keypair.

    :return: A key who's SHA512 digest does not contain 0xFFFF
    """
    while True:
        key = ed25519.Ed25519PrivateKey.generate()
        public_bytes = key.public_key().public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo,
        )

        # The digest don't contain the first byte as it denotes
        # if it is compressed/UncompressedPoint.
        digest = sha512(public_bytes[1:]).digest()[:16]
        if not any([digest[n:n + 2] == b'\xff\xff' for n in range(0, len(digest), 2)]):
            return key


class KeysGeneratorBase(abc.ABC):

    def __init__(self, infile: BinaryIO | None = None) -> None:
        """
        :param infile: A file-like object to read the private key.
        """
        if infile is None:
            self.private_key = self._generate_private_key()
        else:
            self.private_key = load_pem_private_key(infile.read(), password=None)
        self.public_key = self.private_key.public_key()

    @property
    @abc.abstractmethod
    def private_key_pem(self) -> bytes:
        pass

    @property
    @abc.abstractmethod
    def public_key_pem(self) -> bytes:
        pass

    @abc.abstractmethod
    def _generate_private_key(self):
        pass

    @staticmethod
    @abc.abstractmethod
    def sign_message(private_key, message: bytes) -> bytes:
        pass

    @staticmethod
    @abc.abstractmethod
    def verify_signature(
        public_key, message: bytes, signature: bytes
    ) -> bool:
        pass

    def write_private_key_pem(self, outfile: BinaryIO) -> bytes:
        """
        Write private key pem to file and return it.

        :param outfile: A file-like object to write the private key.
        """
        outfile.write(self.private_key_pem)
        return self.private_key_pem

    def write_public_key_pem(self, outfile: BinaryIO) -> bytes:
        """
        Write public key pem to file and return it.

        :param outfile: A file-like object to write the public key.
        """
        outfile.write(self.public_key_pem)
        return self.public_key_pem


class EllipticCurveKeysGenerator(KeysGeneratorBase):
    """Generate private and public keys for Elliptic Curve cryptography."""

    def _generate_private_key(self):
        return generate_legal_key_for_elliptic_curve()

    @property
    def private_key_pem(self) -> bytes:
        return self.private_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.PKCS8,
            encryption_algorithm=serialization.NoEncryption(),
        )

    @property
    def public_key_pem(self) -> bytes:
        return self.public_key.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo,
        )

    @staticmethod
    def verify_signature(
        public_key: ec.EllipticCurvePublicKey, message: bytes, signature: bytes
    ) -> bool:
        try:
            public_key.verify(signature, message, ec.ECDSA(hashes.SHA256()))
            return True
        except InvalidSignature:
            return False

    @staticmethod
    def sign_message(private_key: ec.EllipticCurvePrivateKey, message: bytes) -> bytes:
        return private_key.sign(message, ec.ECDSA(hashes.SHA256()))


class Ed25519KeysGenerator(KeysGeneratorBase):
    """Generate private and public keys for ED25519 cryptography."""

    def _generate_private_key(self):
        return generate_legal_key_for_ed25519()

    @property
    def private_key_pem(self) -> bytes:
        return self.private_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.PKCS8,
            encryption_algorithm=serialization.NoEncryption()
        )

    @property
    def public_key_pem(self) -> bytes:
        return self.public_key.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )

    @staticmethod
    def verify_signature(
        public_key: ed25519.Ed25519PublicKey, message: bytes, signature: bytes
    ) -> bool:
        try:
            public_key.verify(signature, message)
            return True
        except InvalidSignature:
            return False

    @staticmethod
    def sign_message(private_key: ed25519.Ed25519PrivateKey, message: bytes) -> bytes:
        return private_key.sign(message)


ALGORITHMS: dict[str, type[KeysGeneratorBase]] = {
    "ed25519": Ed25519KeysGenerator,
    "ec": EllipticCurveKeysGenerator,
}


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description='Generate PEM file.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False
    )

    priv_pub_group = parser.add_mutually_exclusive_group(required=True)
    priv_pub_group.add_argument('--private', required=False, action='store_true',
                                help='Output a private key.')
    priv_pub_group.add_argument('--public', required=False, action='store_true',
                                help='Output a public key.')
    parser.add_argument('--out', '-out', '-o', required=False,
                        type=argparse.FileType('wb'), default=sys.stdout.buffer,
                        help='Output to specified file instead of stdout.')
    parser.add_argument('--in', '-in', '-i', required=False, dest='infile',
                        type=argparse.FileType('rb'),
                        help='Read private key from specified PEM file instead '
                             'of generating it.')
    parser.add_argument(
        '--algorithm', '-a', help='Signing algorithm (default: %(default)s)',
        required=False, action='store', choices=('ec', 'ed25519'), default='ec'
    )

    args = parser.parse_args(argv)

    try:
        generator = ALGORITHMS[args.algorithm](args.infile)
    except KeyError:
        sys.exit(f'Unknown algorithm {args.algorithm}.')

    if args.private:
        generator.write_private_key_pem(args.out)
    if args.public:
        generator.write_public_key_pem(args.out)
    return 0


if __name__ == '__main__':
    sys.exit(main())
