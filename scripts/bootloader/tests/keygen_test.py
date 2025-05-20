#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import pytest
from cryptography.hazmat.primitives.serialization import (
    load_pem_private_key,
    load_pem_public_key,
)
from keygen import Ed25519KeysGenerator, EllipticCurveKeysGenerator


@pytest.mark.parametrize(
    'keys_generator',
    [EllipticCurveKeysGenerator, Ed25519KeysGenerator],
    ids=['ec', 'ed25519']
)
def test_keys_generator_generates_proper_pem_key(keys_generator):
    key_gen = keys_generator()
    assert b'-----BEGIN PRIVATE KEY-----' in key_gen.private_key_pem
    assert b'-----END PRIVATE KEY-----' in key_gen.private_key_pem
    assert b'-----BEGIN PUBLIC KEY-----' in key_gen.public_key_pem
    assert b'-----END PUBLIC KEY-----' in key_gen.public_key_pem


def test_elliptic_curve_keys_generator(tmpdir):
    private_key_file = tmpdir / 'private.pem'
    ec_keys_generator_1 = EllipticCurveKeysGenerator()
    private_key_pem_1 = ec_keys_generator_1.write_private_key_pem(private_key_file)
    public_key_pem_1 = ec_keys_generator_1.public_key_pem
    # public key and private should not be the same
    assert private_key_pem_1 != public_key_pem_1

    # test if same private key is loaded from file
    ec_keys_generator_2 = EllipticCurveKeysGenerator(private_key_file.open('rb'))
    private_key_pem_2 = ec_keys_generator_2.private_key_pem
    assert private_key_pem_1 == private_key_pem_2


def test_signing_with_elliptic_curve_with_valid_keys(tmpdir, utils):
    private_key_file = tmpdir / 'private.pem'
    public_key_file = tmpdir / 'public.pem'
    generator = EllipticCurveKeysGenerator()
    generator.write_private_key_pem(private_key_file)
    generator.write_public_key_pem(public_key_file)

    message = b'Test message for key verification'
    private_key = load_pem_private_key(utils.read_bytes(private_key_file), password=None)
    public_key = load_pem_public_key(utils.read_bytes(public_key_file))

    signature = EllipticCurveKeysGenerator.sign_message(private_key, message)
    assert EllipticCurveKeysGenerator.verify_signature(public_key, message, signature)


def test_signing_with_elliptic_curve_with_invalid_private_key(tmpdir):
    private_key_file = tmpdir / 'private.pem'
    public_key_file = tmpdir / 'public.pem'
    generator = EllipticCurveKeysGenerator()
    generator.write_private_key_pem(private_key_file)

    public_generator = EllipticCurveKeysGenerator()
    public_generator.write_public_key_pem(public_key_file.open('wb'))

    message = b'Test message for key verification'
    private_key = load_pem_private_key(private_key_file.open('br').read(), password=None)
    public_key = load_pem_public_key(public_key_file.open('br').read())

    signature = EllipticCurveKeysGenerator.sign_message(private_key, message)
    assert EllipticCurveKeysGenerator.verify_signature(public_key, message, signature) is False


def test_ed25519_keys_generator(tmpdir):
    private_key_file = tmpdir / 'private.pem'
    ec_keys_generator_1 = Ed25519KeysGenerator()
    private_key_pem_1 = ec_keys_generator_1.write_private_key_pem(private_key_file)
    public_key_pem_1 = ec_keys_generator_1.public_key_pem
    assert private_key_pem_1 != public_key_pem_1

    # test if same private key is loaded from file
    ec_keys_generator_2 = Ed25519KeysGenerator(private_key_file.open('rb'))
    private_key_pem_2 = ec_keys_generator_2.private_key_pem
    assert private_key_pem_1 == private_key_pem_2


def test_signing_with_ed25519_with_valid_keys(tmpdir, utils):
    private_key_file = tmpdir / 'private.pem'
    public_key_file = tmpdir / 'public.pem'
    generator = Ed25519KeysGenerator()
    generator.write_private_key_pem(private_key_file.open('wb'))
    generator.write_public_key_pem(public_key_file.open('wb'))

    message = b'Test message for key verification'
    private_key = load_pem_private_key(utils.read_bytes(private_key_file), password=None)
    public_key = load_pem_public_key(utils.read_bytes(public_key_file))
    signature = Ed25519KeysGenerator.sign_message(private_key, message)
    assert Ed25519KeysGenerator.verify_signature(public_key, message, signature)


def test_signing_with_ed25519_with_valid_keys_and_private_key_from_public_pem_file(tmpdir, utils):
    private_key_file = tmpdir / 'private.pem'
    public_key_file = tmpdir / 'public.pem'
    private_generator = Ed25519KeysGenerator()
    private_generator.write_private_key_pem(private_key_file.open('wb'))

    public_generator = Ed25519KeysGenerator(private_key_file.open('rb'))
    public_generator.write_public_key_pem(public_key_file.open('wb'))

    message = b'Test message for key verification'
    private_key = load_pem_private_key(utils.read_bytes(private_key_file), password=None)
    public_key = load_pem_public_key(utils.read_bytes(public_key_file))
    signature = Ed25519KeysGenerator.sign_message(private_key, message)
    assert Ed25519KeysGenerator.verify_signature(public_key, message, signature)


def test_signing_with_ed25519_signature_with_invalid_private_key(tmpdir, utils):
    private_key_file = tmpdir / 'private.pem'
    public_key_file = tmpdir / 'public.pem'
    private_generator = Ed25519KeysGenerator()
    private_generator.write_private_key_pem(private_key_file.open('wb'))

    public_generator = Ed25519KeysGenerator()
    public_generator.write_public_key_pem(public_key_file.open('wb'))

    message = b'Test message for key verification'
    private_key = load_pem_private_key(utils.read_bytes(private_key_file), password=None)
    public_key = load_pem_public_key(utils.read_bytes(public_key_file))
    signature = Ed25519KeysGenerator.sign_message(private_key, message)
    assert Ed25519KeysGenerator.verify_signature(public_key, message, signature) is False
