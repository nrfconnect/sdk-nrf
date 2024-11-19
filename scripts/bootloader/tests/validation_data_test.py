#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import hashlib
import textwrap

import ecdsa  # type: ignore[import-untyped]

import do_sign
from hash import generate_hash_digest
from keygen import Ed25519KeysGenerator, EllipticCurveKeysGenerator
from validation_data import (
    Ed25519SignatureValidator,
    EcdsaSignatureValidator,
    main as validation_data_main
)

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


def test_data_validation_for_ec(tmpdir):
    magic_value = '0x281ee6de,0x86518483,79362'
    offset = 0
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
    hash_file.open('wb').write(generate_hash_digest(str(zephyr_hex_file), 'sha256'))

    do_sign.sign_with_ecdsa(private_key_file, hash_file, message_signature_file)

    public_key = ecdsa.VerifyingKey.from_pem(public_key_file.read())
    EcdsaSignatureValidator(hashfunc=hashlib.sha256).append_validation_data(
        signature_file=message_signature_file,
        input_file=zephyr_hex_file,
        public_key=public_key,
        offset=offset,
        output_hex=output_hex_file.open('w'),
        output_bin=output_bin_file.open('wb'),
        magic_value=magic_value
    )
    assert message_signature_file.open('rb').read() in output_bin_file.open('rb').read()
    # check with CLI command too
    assert validation_data_main(
        [
            '--algorithm=ecdsa',
            '--signature', str(message_signature_file),
            '--input', str(zephyr_hex_file),
            '--public-key', str(public_key_file),
            '--offset', offset,
            '--output-hex', str(output_hex_file),
            '--output-bin', str(output_bin_file),
            '--magic-value', magic_value
        ]
    ) == 0
    assert message_signature_file.open('rb').read() in output_bin_file.open('rb').read()


def test_data_validation_for_ed25519(tmpdir):
    magic_value = '0x281ee6de,0x86518483,79362'
    offset = 0
    private_key_file = tmpdir / 'private.pem'
    zephyr_hex_file = tmpdir / 'dummy_zephyr.hex'
    hash_file = tmpdir / 'hash.sha512'
    message_signature_file = tmpdir / 'zephyr.signature'
    output_hex_file = tmpdir / 'output.hex'
    output_bin_file = tmpdir / 'output.bin'

    keys_generator = Ed25519KeysGenerator()
    keys_generator.write_private_key_pem(private_key_file)

    zephyr_hex_file.write(DUMMY_ZEPHYR_HEX)
    hash_file.open('wb').write(generate_hash_digest(str(zephyr_hex_file), 'sha512'))

    do_sign.sign_with_ed25519(private_key_file, hash_file, message_signature_file)

    Ed25519SignatureValidator(hashfunc=hashlib.sha512).append_validation_data(
        signature_file=message_signature_file,
        input_file=zephyr_hex_file,
        public_key=keys_generator.public_key,
        offset=offset,
        output_hex=output_hex_file.open('w'),
        output_bin=output_bin_file.open('wb'),
        magic_value=magic_value
    )
    assert message_signature_file.open('rb').read() in output_bin_file.open('rb').read()


def test_data_validation_for_signed_hex_file_with_ed25519(tmpdir):
    magic_value = '0x281ee6de,0x86518483,79362'
    offset = 0
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
        offset=offset,
        output_hex=output_hex_file.open('w'),
        output_bin=output_bin_file.open('wb'),
        magic_value=magic_value
    )
    assert message_signature_file.open('rb').read() in output_bin_file.open('rb').read()

    # check with CLI command too
    assert validation_data_main(
        [
            '--algorithm=ed25519',
            '--signature', str(message_signature_file),
            '--input', str(zephyr_hex_file),
            '--public-key', str(public_key_file),
            '--offset', offset,
            '--output-hex', str(output_hex_file),
            '--output-bin', str(output_bin_file),
            '--magic-value', magic_value
        ]
    ) == 0
    assert message_signature_file.open('rb').read() in output_bin_file.open('rb').read()
