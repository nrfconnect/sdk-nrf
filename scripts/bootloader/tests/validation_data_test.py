#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import hashlib
import textwrap

import do_sign
from asn1parse import get_ecdsa_signature
from cryptography.hazmat.primitives.serialization import load_pem_public_key
from hash import generate_hash_digest
from keygen import Ed25519KeysGenerator, EllipticCurveKeysGenerator
from validation_data import EcdsaSignatureValidator, Ed25519SignatureValidator
from validation_data import main as validation_data_main

MAGIC_VALUE = '0x281ee6de,0x86518483,79362'
OFFSET = 0
DUMMY_ZEPHYR_HEX = textwrap.dedent("""\
    :1098000050110020EDA60000E5DE0000D9A6000002
    :10981000D9A60000D9A60000D9A60000D9A600004C
    :1098200000000000000000000000000099A80000F7
    :10983000D9A600000000000059A80000D9A6000029
    :1098400001AA000001AA000001AA000001AA00006C
    :1098500001AA000001AA000001AA000001AA00005C
    :1098600001AA000001AA000001AA000001AA00004C
    :1098700001AA000001AA000001AA000001AA00003C
    :1098800001AA000001AA000001AA000001AA00002C
    :1098900001AA000001AA000001AA000001AA00001C
    :08F5040050010020500100201D
    :0AF50C0000000000000000000000F5
    :04F5160015E015E007
    :040000030000A6ED66
    :00000001FF
""")


def load_ecdsa_public_key(public_key_file: str):
    with open(public_key_file, 'rb') as f:
        return load_pem_public_key(f.read())


def load_ecdsa_public_key_from_string(public_key_string: bytes):
    return load_pem_public_key(public_key_string)


def test_data_validation_for_ec(tmpdir, utils):
    private_key_file = tmpdir / 'private.pem'
    public_key_file = tmpdir / 'public.pem'
    zephyr_hex_file = tmpdir / 'dummy_zephyr.hex'
    message_signature_file = tmpdir / 'zephyr.signature'
    output_hex_file = tmpdir / 'output.hex'
    output_bin_file = tmpdir / 'output.bin'
    hash_file = tmpdir / 'hash.256'

    keys_generator = EllipticCurveKeysGenerator()
    keys_generator.write_private_key_pem(private_key_file)
    keys_generator.write_public_key_pem(public_key_file)

    zephyr_hex_file.write(DUMMY_ZEPHYR_HEX)
    hash_digest = generate_hash_digest(str(zephyr_hex_file), 'sha256')
    utils.write_bytes(hash_file, hash_digest)

    do_sign.sign_with_ecdsa(private_key_file, hash_file, message_signature_file)

    public_key = load_ecdsa_public_key(public_key_file)
    EcdsaSignatureValidator(hashfunc=hashlib.sha256).append_validation_data(
        signature_file=message_signature_file,
        input_file=zephyr_hex_file,
        public_key=public_key,
        offset=OFFSET,
        output_hex=output_hex_file.open('w'),
        output_bin=output_bin_file.open('wb'),
        magic_value=MAGIC_VALUE
    )
    assert utils.read_bytes(message_signature_file) in utils.read_bytes(output_bin_file)
    # check with CLI command too
    assert validation_data_main(
        [
            '--algorithm=ecdsa',
            '--signature', str(message_signature_file),
            '--input', str(zephyr_hex_file),
            '--public-key', str(public_key_file),
            '--offset', OFFSET,
            '--output-hex', str(output_hex_file),
            '--output-bin', str(output_bin_file),
            '--magic-value', MAGIC_VALUE
        ]
    ) == 0
    assert utils.read_bytes(message_signature_file) in utils.read_bytes(output_bin_file)


def test_data_validation_for_ec_with_openssl(tmpdir, utils):
    private_key_file = tmpdir / 'private.pem'
    public_key_file = tmpdir / 'public.pem'
    zephyr_hex_file = tmpdir / 'dummy_zephyr.hex'
    message_signature_file = tmpdir / 'zephyr.signature'
    output_hex_file = tmpdir / 'output.hex'
    output_bin_file = tmpdir / 'output.bin'
    hash_file = tmpdir / 'hash.256'
    asn1_signature_file = tmpdir / 'asn1.signature'

    utils.openssl_generate_private_key(private_key_file)
    utils.openssl_generate_public_key(private_key_file, public_key_file)

    zephyr_hex_file.write(DUMMY_ZEPHYR_HEX)

    # Generate sha256 hash
    hash_digest = generate_hash_digest(str(zephyr_hex_file), 'sha256')
    utils.write_bytes(hash_file, hash_digest)

    # sign hex file
    utils.openssl_sign_message(private_key_file, hash_file, message_signature_file)
    asn1_signature = get_ecdsa_signature(utils.read_bytes(message_signature_file), 32)
    utils.write_bytes(asn1_signature_file, asn1_signature)

    # create validation data and verify signature
    public_key = load_ecdsa_public_key(public_key_file)
    EcdsaSignatureValidator(hashfunc=hashlib.sha256).append_validation_data(
        signature_file=asn1_signature_file,
        input_file=zephyr_hex_file,
        public_key=public_key,
        offset=OFFSET,
        output_hex=output_hex_file.open('w'),
        output_bin=output_bin_file.open('wb'),
        magic_value=MAGIC_VALUE
    )
    assert utils.read_bytes(asn1_signature_file) in utils.read_bytes(output_bin_file)
    # check with CLI command too
    assert validation_data_main(
        [
            '--algorithm=ecdsa',
            '--signature', str(asn1_signature_file),
            '--input', str(zephyr_hex_file),
            '--public-key', str(public_key_file),
            '--offset', OFFSET,
            '--output-hex', str(output_hex_file),
            '--output-bin', str(output_bin_file),
            '--magic-value', MAGIC_VALUE
        ]
    ) == 0
    assert utils.read_bytes(asn1_signature_file) in utils.read_bytes(output_bin_file)


def test_data_validation_for_ed25519_with_sha512(tmpdir, utils):
    private_key_file = tmpdir / 'private.pem'
    zephyr_hex_file = tmpdir / 'dummy_zephyr.hex'
    hash_file = tmpdir / 'hash.sha512'
    message_signature_file = tmpdir / 'zephyr.signature'
    output_hex_file = tmpdir / 'output.hex'
    output_bin_file = tmpdir / 'output.bin'

    keys_generator = Ed25519KeysGenerator()
    keys_generator.write_private_key_pem(private_key_file)

    zephyr_hex_file.write(DUMMY_ZEPHYR_HEX)
    hash_digits = generate_hash_digest(str(zephyr_hex_file), 'sha512')
    utils.write_bytes(hash_file, hash_digits)

    do_sign.sign_with_ed25519(private_key_file, hash_file, message_signature_file)

    Ed25519SignatureValidator(hashfunc=hashlib.sha512).append_validation_data(
        signature_file=message_signature_file,
        input_file=zephyr_hex_file,
        public_key=keys_generator.public_key,
        offset=OFFSET,
        output_hex=output_hex_file.open('w'),
        output_bin=output_bin_file.open('wb'),
        magic_value=MAGIC_VALUE
    )
    assert utils.read_bytes(message_signature_file) in utils.read_bytes(output_bin_file)


def test_data_validation_for_ed25519_no_hash(tmpdir, utils):
    private_key_file = tmpdir / 'private.pem'
    public_key_file = tmpdir / 'public.pem'
    zephyr_hex_file = tmpdir / 'dummy_zephyr.hex'
    message_signature_file = tmpdir / 'zephyr.signature'
    output_hex_file = tmpdir / 'output.hex'
    output_bin_file = tmpdir / 'output.bin'

    keys_generator = Ed25519KeysGenerator()
    keys_generator.write_private_key_pem(private_key_file)
    keys_generator.write_public_key_pem(public_key_file)

    zephyr_hex_file.write(DUMMY_ZEPHYR_HEX)
    do_sign.sign_with_ed25519(private_key_file, zephyr_hex_file, message_signature_file)

    # verify signature
    Ed25519SignatureValidator().verify(
        public_key=keys_generator.public_key,
        message_bytes=do_sign.hex_to_binary(str(zephyr_hex_file)),
        signature_bytes=message_signature_file.open('rb').read()
    )

    # create validation data
    Ed25519SignatureValidator().append_validation_data(
        signature_file=message_signature_file,
        input_file=zephyr_hex_file,
        public_key=keys_generator.public_key,
        offset=OFFSET,
        output_hex=output_hex_file.open('w'),
        output_bin=output_bin_file.open('wb'),
        magic_value=MAGIC_VALUE
    )
    assert utils.read_bytes(message_signature_file) in utils.read_bytes(output_bin_file)

    # check with CLI command too
    assert validation_data_main(
        [
            '--algorithm=ed25519',
            '--signature', str(message_signature_file),
            '--input', str(zephyr_hex_file),
            '--public-key', str(public_key_file),
            '--offset', OFFSET,
            '--output-hex', str(output_hex_file),
            '--output-bin', str(output_bin_file),
            '--magic-value', MAGIC_VALUE
        ]
    ) == 0
    assert utils.read_bytes(message_signature_file) in utils.read_bytes(output_bin_file)


def test_ecdsa_public_key_to_string(tmpdir):
    public_key_data = textwrap.dedent("""\
        -----BEGIN PUBLIC KEY-----
        MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEihHfHl4KigTP4uFhlo1M1IkezteI
        DLYmPEVtPrPYrQtqpctToFIG8uV3g6MAMhqPG/h69polPuvN3Lo56YnXhA==
        -----END PUBLIC KEY-----
    """)
    public_key = load_ecdsa_public_key_from_string(public_key_data.encode())
    validator = EcdsaSignatureValidator(hashfunc=hashlib.sha256)
    expected_bytes = (b'\x8a\x11\xdf\x1e^\n\x8a\x04\xcf\xe2\xe1a\x96\x8dL\xd4\x89\x1e'
                      b'\xce\xd7\x88\x0c\xb6&<Em>\xb3\xd8\xad\x0bj\xa5\xcbS\xa0R\x06'
                      b'\xf2\xe5w\x83\xa3\x002\x1a\x8f\x1b\xf8z\xf6\x9a%>\xeb\xcd\xdc'
                      b'\xba9\xe9\x89\xd7\x84')
    assert validator.to_string(public_key) == expected_bytes


def test_ed25519_public_key_to_string(tmpdir):
    public_key_data = textwrap.dedent("""\
        -----BEGIN PUBLIC KEY-----
        MCowBQYDK2VwAyEAHq4VC8pj/yLm8a7IzOZfkjB4o6Vk6UvRGGKAgpl24q8=
        -----END PUBLIC KEY-----
    """)
    public_key = load_pem_public_key(public_key_data.encode())
    validator = Ed25519SignatureValidator()
    expected_bytes = (b'\x1e\xae\x15\x0b\xcac\xff"\xe6\xf1\xae\xc8\xcc\xe6_\x920x'
                      b'\xa3\xa5d\xe9K\xd1\x18b\x80\x82\x99v\xe2\xaf')
    assert validator.to_string(public_key) == expected_bytes
