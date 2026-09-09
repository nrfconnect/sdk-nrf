#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''Optional KMS signing keys, addressed by cosign-style URIs.

To add a provider, register its URI scheme in _PROVIDERS. Its constructor accepts the URI,
loads its SDK lazily, and exposes a cryptography public_key. Its sign_digest(digest,
digest_name) method returns a signature over the precomputed digest (DER for ECDSA,
PKCS #1 v1.5 for RSA). Provider failures must raise SbomException; _kms_errors can wrap
SDK calls. Key types, hashes, bundles and signature verification belong to sign_spdx.
'''

import re
from contextlib import contextmanager

from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from sbom_exceptions import SbomException

_AWS_UUID = (
    r'm?r?k?-?[A-Fa-f0-9]{8}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{4}-?[A-Fa-f0-9]{12}'
)
_AWS_ARN = r'arn:(?:aws|aws-us-gov|aws-cn):kms:[a-z0-9-]+:\d{12}:'
_AWS_REFERENCE = re.compile(
    rf'awskms://([^/]*)/({_AWS_UUID}|{_AWS_ARN}key/{_AWS_UUID}|(?:{_AWS_ARN})?alias/.+)'
)


@contextmanager
def _kms_errors(reference: str):
    '''Report provider SDK failures through the SBOM command's error handler.'''
    try:
        yield
    except SbomException:
        raise
    except Exception as ex:
        raise SbomException(f'KMS operation failed for {reference}: {ex}') from ex


class AwsKey:
    '''AWS KMS key: awskms://[ENDPOINT]/[KEY ID, KEY ARN, ALIAS NAME or ALIAS ARN].'''

    def __init__(self, reference: str):
        match = _AWS_REFERENCE.fullmatch(reference)
        if match is None:
            raise SbomException(
                f'Invalid AWS KMS key URI: {reference}. '
                'Expected awskms://[ENDPOINT]/[KEY ID, KEY ARN, ALIAS NAME or ALIAS ARN].'
            )
        try:
            import boto3
        except ImportError as ex:
            raise SbomException(
                'AWS KMS keys require the "boto3" Python package.\n'
                'Install the SBOM requirements with:\n'
                '  pip3 install -r scripts/requirements-west-ncs-sbom.txt'
            ) from ex

        self._reference = reference
        endpoint, key_id = match.groups()
        options = {}
        if key_id.startswith('arn:'):
            options['region_name'] = key_id.split(':')[3]
        if endpoint:
            options['endpoint_url'] = f'https://{endpoint}'
        with _kms_errors(reference):
            self._client = boto3.client('kms', **options)
            response = self._client.get_public_key(KeyId=key_id)
            self.public_key = serialization.load_der_public_key(response['PublicKey'])
            # Keep using the loaded key even if its alias is reassigned during this command.
            self._key_id = response['KeyId']

    def sign_digest(self, digest: bytes, digest_name: str) -> bytes:
        if isinstance(self.public_key, rsa.RSAPublicKey):
            algorithm = f'RSASSA_PKCS1_V1_5_SHA_{digest_name[3:]}'
        else:
            algorithm = f'ECDSA_SHA_{digest_name[3:]}'
        with _kms_errors(self._reference):
            response = self._client.sign(
                KeyId=self._key_id, Message=digest, MessageType='DIGEST', SigningAlgorithm=algorithm
            )
            return response['Signature']


_PROVIDERS = {'awskms': AwsKey}


def open_key(reference: str):
    '''Open a supported KMS key; new providers only need an entry in _PROVIDERS.'''
    scheme, separator, _ = reference.partition('://')
    key_class = _PROVIDERS.get(scheme) if separator else None
    if key_class is None:
        supported = ', '.join(f'{scheme}://' for scheme in _PROVIDERS)
        raise SbomException(f'Unsupported KMS key URI: {reference}. Supported schemes: {supported}')
    return key_class(reference)
