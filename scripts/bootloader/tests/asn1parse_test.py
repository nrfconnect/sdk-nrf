#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import subprocess

from asn1parse import get_ecdsa_signature
from keygen import EllipticCurveKeysGenerator


def test_asn1parse_with_ecdsa_and_sha256(tmpdir):
    signature_der_file = tmpdir / 'signature.bin'
    private_key_file = tmpdir / 'private.pem'
    public_key_file = tmpdir / 'public.pem'
    generator = EllipticCurveKeysGenerator()
    generator.write_private_key_pem(private_key_file)
    generator.write_public_key_pem(public_key_file)

    input_file = tmpdir / 'input_file.txt'
    message = b'Test message for key verification'
    input_file.write(message)

    subprocess.run(
        [
            'openssl', 'dgst', '-sha256', '-sign', private_key_file,
            '-out', signature_der_file, input_file
        ],
        check=True
    )
    assert signature_der_file.exists()

    signature = get_ecdsa_signature(signature_der_file.open('rb').read(), clength=32)
    assert len(signature) == 64
    result = subprocess.run(
        [
            'openssl', 'dgst', '-sha256', '-verify', public_key_file,
            '-signature', signature_der_file, input_file
        ]
    )
    assert result.returncode == 0, 'Signature does not match'

    input_file.write(b'Test message to fail verification')
    result = subprocess.run(
        [
            'openssl', 'dgst', '-sha256', '-verify', public_key_file,
            '-signature', signature_der_file, input_file
        ],
    )
    assert result.returncode == 1
