#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
from cryptography.hazmat.primitives.serialization import load_pem_public_key
from do_sign import sign_with_ecdsa, sign_with_ed25519
from keygen import Ed25519KeysGenerator, EllipticCurveKeysGenerator


def test_if_file_is_properly_signed_with_ec_key(tmpdir, utils):
    generator = EllipticCurveKeysGenerator()
    private_key_file = tmpdir / 'private.pem'
    generator.write_private_key_pem(private_key_file)
    public_key_file = tmpdir / 'public.pem'
    generator.write_public_key_pem(public_key_file)

    input_file = tmpdir / 'input_file.txt'
    message = b'Test message for key verification'
    input_file.write(message)

    signature_file = tmpdir / 'signature.bin'

    sign_with_ecdsa(
        private_key_file=private_key_file,
        input_file=input_file,
        output_file=signature_file,
    )

    public_key = load_pem_public_key(utils.read_bytes(public_key_file))
    assert utils.verify_ec_signature(
        public_key, message, utils.read_bytes(signature_file)
    )


def test_if_validation_does_not_pass_for_wrong_ec_key(tmpdir, utils):
    private_key_file = tmpdir / 'private.pem'
    EllipticCurveKeysGenerator().write_private_key_pem(private_key_file)
    public_key_file = tmpdir / 'public.pem'
    EllipticCurveKeysGenerator().write_public_key_pem(public_key_file)

    input_file = tmpdir / 'input_file.txt'
    message = b'Test message for key verification'
    input_file.write(message)

    signature_file = tmpdir / 'signature.bin'

    sign_with_ecdsa(
        private_key_file=private_key_file,
        input_file=input_file,
        output_file=signature_file,
    )

    public_key = load_pem_public_key(utils.read_bytes(public_key_file))
    assert EllipticCurveKeysGenerator.verify_signature(
        public_key, message, utils.read_bytes(signature_file)
    ) is False


def test_if_validation_does_not_pass_for_wrong_ed25519_key(tmpdir, utils):
    generator = Ed25519KeysGenerator()
    private_key_file = tmpdir / 'private.pem'
    generator.write_private_key_pem(private_key_file)
    public_key = generator.public_key

    input_file = tmpdir / 'input_file.txt'
    message = b'Test message for key verification'
    input_file.write(message)

    signature_file = tmpdir / 'signature.bin'

    sign_with_ed25519(
        private_key_file=private_key_file,
        input_file=input_file,
        output_file=signature_file
    )
    assert Ed25519KeysGenerator.verify_signature(
        public_key, message, utils.read_bytes(signature_file)
    )


def test_if_file_is_properly_signed_with_ed25519_key(tmpdir, utils):
    private_key_file = tmpdir / 'private.pem'
    Ed25519KeysGenerator().write_private_key_pem(private_key_file)
    public_key = Ed25519KeysGenerator().public_key

    input_file = tmpdir / 'input_file.txt'
    message = b'Test message for key verification'
    input_file.write(message)

    signature_file = tmpdir / 'signature.bin'

    sign_with_ed25519(
        private_key_file=private_key_file,
        input_file=input_file,
        output_file=signature_file
    )
    assert Ed25519KeysGenerator.verify_signature(
        public_key, message, utils.read_bytes(signature_file)
    ) is False
