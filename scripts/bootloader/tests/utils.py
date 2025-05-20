import subprocess

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric.utils import encode_dss_signature


def write_bytes(filename: str, content: bytes) -> None:
    with open(filename, 'wb') as f:
        f.write(content)


def read_bytes(filename: str) -> bytes:
    with open(filename, 'rb') as f:
        return f.read()


def verify_ec_signature(
    public_key: ec.EllipticCurvePublicKey, message: bytes, signature: bytes
) -> bool:
    assert len(signature) == 64
    values = signature[:32], signature[32:]
    key1, key2 = (int.from_bytes(v, byteorder='big') for v in values)
    signature_encoded = encode_dss_signature(key1, key2)
    try:
        public_key.verify(signature_encoded, message, ec.ECDSA(hashes.SHA256()))
        return True
    except InvalidSignature:
        return False


def openssl_generate_public_key(private_key_file, public_key_file):
    return subprocess.run(
        [
            'openssl', 'ec', '-in', str(private_key_file), '-pubout',
            '-out', str(public_key_file)
        ],
        check=True,
    )


def openssl_generate_private_key(private_key_file):
    return subprocess.run(
        [
            'openssl', 'ecparam', '-name', 'secp256r1', '-genkey',
            '-noout', '-out', str(private_key_file)
        ],
        check=True
    )


def openssl_sign_message(public_key_file, input_file, output_file):
    return subprocess.run(
        [
            'openssl', 'dgst', '-sha256', '-sign', str(public_key_file), str(input_file)
        ],
        check=True,
        stdout=output_file.open('wb')
    )
