#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''Private SPDX signing and verification with Sigstore bundles that cosign also accepts.

Signing uses a key file or a KMS key and contacts no certificate authority, transparency log or
timestamp authority. The bundle matches what "cosign sign-blob --key" writes when those services
are disabled, so "cosign verify-blob --key --insecure-ignore-tlog" verifies it as well.
'''

import base64
import getpass
import hashlib
import json
import os
import re
import sys
import warnings
from pathlib import Path

import sign_kms
from cryptography.exceptions import InvalidSignature, UnsupportedAlgorithm
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec, padding, rsa, utils
from sbom_exceptions import SbomException
from west import log

BUNDLE_SUFFIX = '.sigstore.json'
BUNDLE_MEDIA_TYPE = 'application/vnd.dev.sigstore.bundle.v0.3+json'
_ACCEPTED_MEDIA_TYPES = (BUNDLE_MEDIA_TYPE, 'application/vnd.dev.sigstore.bundle+json;version=0.3')

# cosign picks the hash from the key type (GetDefaultPublicKeyDetails in sigstore), and its
# verify-blob expects that hash, so every key, including KMS keys, signs with the same choice.
_EC_DIGESTS = {'secp256r1': 'sha256', 'secp384r1': 'sha384', 'secp521r1': 'sha512'}
_RSA_KEY_SIZES = (2048, 3072, 4096)
_HASHES = {'sha256': hashes.SHA256, 'sha384': hashes.SHA384, 'sha512': hashes.SHA512}
_BUNDLE_DIGESTS = {'sha256': 'SHA2_256', 'sha384': 'SHA2_384', 'sha512': 'SHA2_512'}

PASSWORD_VARIABLE = 'COSIGN_PASSWORD'
_COSIGN_KEY_TYPES = ('ENCRYPTED SIGSTORE PRIVATE KEY', 'ENCRYPTED COSIGN PRIVATE KEY')
_PEM_KEY_TYPES = ('ENCRYPTED PRIVATE KEY', 'PRIVATE KEY', 'EC PRIVATE KEY', 'RSA PRIVATE KEY')
_PEM_BLOCK = re.compile(rb'-----BEGIN ([A-Z0-9 ]+)-----\r?\n(.*?)-----END \1-----', re.DOTALL)
# The scrypt settings that cosign writes and accepts (go-securesystemslib "encrypted").
_SCRYPT_PARAMETERS = ((32768, 8, 1), (65536, 8, 1), (131072, 8, 1))


def bundle_path(report: 'str|Path') -> Path:
    '''Return the bundle path for a report: the report name with ".sigstore.json" appended.'''
    return Path(str(report) + BUNDLE_SUFFIX)


def signature_digest(public_key) -> str:
    '''Return the hashlib name of the digest that cosign signs for this key type.'''
    if isinstance(public_key, ec.EllipticCurvePublicKey) and public_key.curve.name in _EC_DIGESTS:
        return _EC_DIGESTS[public_key.curve.name]
    if isinstance(public_key, rsa.RSAPublicKey) and public_key.key_size in _RSA_KEY_SIZES:
        return 'sha256'
    raise SbomException(
        'Unsupported signing key. Use an ECDSA P-256, P-384 or P-521 key, '
        'or an RSA 2048, 3072 or 4096-bit key.'
    )


def _with_scheme(method, public_key, digest_name: str, *leading):
    '''Call a key's sign() or verify() method with the scheme arguments for a precomputed digest.'''
    prehashed = utils.Prehashed(_HASHES[digest_name]())
    if isinstance(public_key, ec.EllipticCurvePublicKey):
        return method(*leading, ec.ECDSA(prehashed))
    return method(*leading, padding.PKCS1v15(), prehashed)


def _signature_valid(public_key, signature: bytes, digest: bytes, digest_name: str) -> bool:
    try:
        _with_scheme(public_key.verify, public_key, digest_name, signature, digest)
    except InvalidSignature:
        return False
    return True


def public_key_hint(public_key) -> str:
    '''Return the key hint that cosign records: base64 of the SHA-256 of the DER public key.'''
    der = public_key.public_bytes(
        serialization.Encoding.DER, serialization.PublicFormat.SubjectPublicKeyInfo
    )
    return base64.b64encode(hashlib.sha256(der).digest()).decode()


def _read_passphrase() -> bytes:
    '''Read the key passphrase from COSIGN_PASSWORD, or prompt for it without echo.'''
    passphrase = os.environ.get(PASSWORD_VARIABLE)
    if passphrase is None:
        if not sys.stdin.isatty():
            raise SbomException(
                f'Set {PASSWORD_VARIABLE} to sign with an encrypted key in a non-interactive '
                'environment.'
            )
        try:
            with warnings.catch_warnings():
                warnings.simplefilter('error', getpass.GetPassWarning)
                passphrase = getpass.getpass('Signing key passphrase: ')
        except (EOFError, KeyboardInterrupt, getpass.GetPassWarning) as ex:
            raise SbomException(
                f'Cannot read signing key passphrase securely; set {PASSWORD_VARIABLE} instead.'
            ) from ex
    return passphrase.encode()


def _decrypt_cosign_key(body: bytes) -> bytes:
    '''Decrypt a cosign key file: scrypt derives the NaCl secretbox key that seals the PKCS #8
    private key. Only cosign key files need pynacl, so it is imported here, before the passphrase
    is requested.'''
    try:
        envelope = json.loads(base64.b64decode(b''.join(body.split())))
        kdf, cipher = envelope['kdf'], envelope['cipher']
        parameters = (kdf['params']['N'], kdf['params']['r'], kdf['params']['p'])
        algorithms = (kdf['name'], cipher['name'])
        salt = base64.b64decode(kdf['salt'])
        nonce = base64.b64decode(cipher['nonce'])
        ciphertext = base64.b64decode(envelope['ciphertext'])
    except (ValueError, KeyError, TypeError) as ex:
        raise SbomException('Invalid cosign key file') from ex
    if algorithms != ('scrypt', 'nacl/secretbox') or parameters not in _SCRYPT_PARAMETERS:
        raise SbomException('Unsupported cosign key file encryption')
    try:
        import nacl.exceptions
        import nacl.secret
    except ImportError as ex:
        raise SbomException(
            f'Cosign key files require the "pynacl" Python package ({ex}).\n'
            'Install the SBOM requirements with:\n'
            '  pip3 install -r scripts/requirements-west-ncs-sbom.txt'
        ) from ex
    n, r, p = parameters
    key = hashlib.scrypt(
        _read_passphrase(),
        salt=salt,
        n=n,
        r=r,
        p=p,
        maxmem=256 * r * (n + p),
        dklen=nacl.secret.SecretBox.KEY_SIZE,
    )
    try:
        return nacl.secret.SecretBox(key).decrypt(ciphertext, nonce)
    except nacl.exceptions.CryptoError as ex:
        raise SbomException('Cannot decrypt the signing key: wrong passphrase') from ex


def load_private_key(path: Path):
    '''Load a cosign key file or a PEM private key, asking for a passphrase only when the key is
    encrypted.'''
    try:
        data = path.read_bytes()
    except OSError as ex:
        raise SbomException(f'Cannot read signing key file {path}: {ex}') from ex
    try:
        # OpenSSL key files can include other PEM blocks, such as EC PARAMETERS.
        for block in _PEM_BLOCK.finditer(data):
            kind = block[1].decode()
            if kind in _COSIGN_KEY_TYPES:
                der = _decrypt_cosign_key(block[2])
                return serialization.load_der_private_key(der, None)
            if kind in _PEM_KEY_TYPES:
                encrypted = kind == 'ENCRYPTED PRIVATE KEY' or b'Proc-Type: 4,ENCRYPTED' in block[2]
                return serialization.load_pem_private_key(
                    block[0], _read_passphrase() if encrypted else None
                )
    except (ValueError, TypeError, UnsupportedAlgorithm) as ex:
        raise SbomException(
            f'Cannot load signing key {path}: wrong passphrase or invalid key ({ex})'
        ) from ex
    raise SbomException(f'Signing key file {path} is not a cosign or PEM private key')


class _FileKey:
    '''Private key loaded from a key file.'''

    def __init__(self, path: Path):
        self._private_key = load_private_key(path)
        self.public_key = self._private_key.public_key()

    def sign_digest(self, digest: bytes, digest_name: str) -> bytes:
        return _with_scheme(self._private_key.sign, self.public_key, digest_name, digest)


def public_key_pem(public_key) -> bytes:
    return public_key.public_bytes(
        serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo
    )


class SpdxSigner:
    '''Load the signing key once, then sign each generated SPDX report.'''

    def __init__(self, key: str):
        self._key_file = None
        if '://' in key:
            self._key = sign_kms.open_key(key)
        else:
            self._key_file = Path(key)
            self._key = _FileKey(self._key_file)
        self.public_key = self._key.public_key
        self._digest_name = signature_digest(self.public_key)
        self._hint = public_key_hint(self.public_key)

    def export_public_key(self, path: 'str|Path'):
        '''Write the PEM public key that recipients need to verify the signatures.'''
        self.check_not_key(path)
        try:
            Path(path).write_bytes(public_key_pem(self.public_key))
        except OSError as ex:
            raise SbomException(f'Cannot write public key {path}: {ex}') from ex
        log.inf(f'Public key: {path}')

    def check_not_key(self, path: 'str|Path'):
        '''Refuse to write over the signing key file, including through symlinks and hard links.'''
        if (
            self._key_file is not None
            and Path(path).exists()
            and os.path.samefile(path, self._key_file)
        ):
            raise SbomException(f'Refusing to write {path}: it is the signing key file')

    def remove_bundle(self, output_file: 'str|Path'):
        '''Delete the bundle of a report that is about to be regenerated, so that a failure
        cannot leave an old signature beside a partial report.'''
        self.check_not_key(output_file)
        bundle = bundle_path(Path(output_file).absolute())
        self.check_not_key(bundle)
        try:
            bundle.unlink(missing_ok=True)
        except OSError as ex:
            raise SbomException(
                f'Cannot remove the old signature bundle {bundle}: {ex}. '
                'The report was not changed.'
            ) from ex

    def sign(self, output_file: 'str|Path'):
        '''Write a bundle beside the completed SPDX file; remove stale bundles on failure.'''
        # Preserve symlink names so verification finds the bundle beside the supplied path.
        output = Path(output_file).absolute()
        bundle = bundle_path(output)
        try:
            # The SPDX file has been regenerated, so its previous signature is obsolete.
            bundle.unlink(missing_ok=True)
            digest = hashlib.new(self._digest_name, output.read_bytes()).digest()
            signature = self._key.sign_digest(digest, self._digest_name)
            # Check the provider's signature before writing a bundle for recipients.
            if not _signature_valid(self.public_key, signature, digest, self._digest_name):
                raise SbomException('the key produced a signature that does not verify')
            content = {
                'mediaType': BUNDLE_MEDIA_TYPE,
                'verificationMaterial': {'publicKey': {'hint': self._hint}},
                'messageSignature': {
                    'messageDigest': {
                        'algorithm': _BUNDLE_DIGESTS[self._digest_name],
                        'digest': base64.b64encode(digest).decode(),
                    },
                    'signature': base64.b64encode(signature).decode(),
                },
            }
            try:
                bundle.write_bytes(json.dumps(content, indent=2).encode() + b'\n')
            except OSError:
                bundle.unlink(missing_ok=True)
                raise
        except (OSError, SbomException) as ex:
            raise SbomException(
                f'Signing failed for {output}: {ex}. No signature bundle was produced.'
            ) from ex
        log.inf(f'Signature bundle: {bundle}')


def load_public_key(reference: str):
    '''Load a PEM public key file, or fetch the public key of a KMS key.'''
    if '://' in reference:
        return sign_kms.open_key(reference).public_key
    try:
        return serialization.load_pem_public_key(Path(reference).read_bytes())
    except (OSError, ValueError, UnsupportedAlgorithm) as ex:
        raise SbomException(f'Cannot load public key {reference}: {ex}') from ex


def verify_spdx(report: 'str|Path', public_key):
    '''Check a report against its bundle, like "cosign verify-blob --key KEY --bundle BUNDLE
    --insecure-ignore-tlog". Transparency log entries in the bundle, if any, are not checked.'''
    bundle = bundle_path(report)
    try:
        data = Path(report).read_bytes()
        content = json.loads(Path(bundle).read_bytes())
    except (OSError, ValueError) as ex:
        raise SbomException(f'Cannot read {report} or its bundle: {ex}') from ex
    try:
        media_type = content['mediaType']
        key_material = content['verificationMaterial']['publicKey']
        message = content['messageSignature']
        algorithm = message['messageDigest']['algorithm']
        signed_digest = base64.b64decode(message['messageDigest']['digest'])
        signature = base64.b64decode(message['signature'])
    except (KeyError, TypeError, ValueError) as ex:
        raise SbomException(f'{bundle} is not a key-signed Sigstore bundle') from ex
    if media_type not in _ACCEPTED_MEDIA_TYPES or not isinstance(key_material, dict):
        raise SbomException(f'{bundle} is not a key-signed Sigstore v0.3 bundle')
    digest_name = signature_digest(public_key)
    if algorithm != _BUNDLE_DIGESTS[digest_name]:
        raise SbomException(
            f'{bundle} uses {algorithm}, but the public key signs {_BUNDLE_DIGESTS[digest_name]}'
        )
    if hashlib.new(digest_name, data).digest() != signed_digest:
        raise SbomException(f'{report} does not match the digest signed in {bundle}')
    if not _signature_valid(public_key, signature, signed_digest, digest_name):
        raise SbomException(f'The signature in {bundle} was not made with the given key')


def verify_reports(reports: 'list[str]', key: str):
    '''Verify each report against its bundle and fail if any of them does not verify.'''
    public_key = load_public_key(key)
    failures = 0
    for report in reports:
        try:
            verify_spdx(report, public_key)
        except SbomException as ex:
            log.err(str(ex))
            failures += 1
        else:
            log.inf(f'Verified OK: {report}')
    if failures:
        raise SbomException(f'{failures} of {len(reports)} SPDX reports failed verification')
