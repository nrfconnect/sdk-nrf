#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''Private SPDX signing with cosign local keys or KMS references.'''

import getpass
import os
import subprocess
import sys
import warnings
from pathlib import Path
from tempfile import TemporaryDirectory

from helpers.install_cosign import CosignError, ensure_cosign
from sbom_exceptions import SbomException
from west import log

DATA_DIR = Path(__file__).parent / 'data'


class SpdxSigner:
    '''Resolve cosign and credentials once, then sign each generated SPDX report.'''

    def __init__(self, key: str, executable: 'str|None' = None):
        local_key = '://' not in key
        if local_key and not Path(key).is_file():
            raise SbomException(f'Signing key file not found: {key}')
        try:
            self.executable = ensure_cosign(executable)
        except CosignError as ex:
            raise SbomException(str(ex)) from ex
        self.key = key
        # Keep a prompted password in the child environment, never in os.environ or argv.
        self.env = os.environ.copy()
        if local_key and 'COSIGN_PASSWORD' not in self.env:
            if not sys.stdin.isatty():
                raise SbomException(
                    'Set COSIGN_PASSWORD to sign with a local key in a non-interactive environment.'
                )
            try:
                with warnings.catch_warnings():
                    warnings.simplefilter('error', getpass.GetPassWarning)
                    self.env['COSIGN_PASSWORD'] = getpass.getpass('Signing key passphrase: ')
            except (EOFError, KeyboardInterrupt, getpass.GetPassWarning) as ex:
                raise SbomException(
                    'Cannot read signing key passphrase securely; set COSIGN_PASSWORD instead.'
                ) from ex

    def sign(self, output_file: 'str|Path'):
        '''Write a bundle beside the completed SPDX file; remove stale bundles on failure.'''
        output = Path(output_file).resolve()
        bundle = Path(str(output) + '.sigstore.json')
        try:
            # The SPDX file has been regenerated, so its previous signature is obsolete.
            bundle.unlink(missing_ok=True)
            with TemporaryDirectory(prefix='.sbom-sign-', dir=output.parent) as directory:
                temporary_bundle = Path(directory) / 'bundle.json'
                subprocess.run(
                    [
                        self.executable,
                        'sign-blob',
                        '--key',
                        self.key,
                        '--signing-config',
                        str(DATA_DIR / 'cosign-signing-config.json'),
                        '--trusted-root',
                        str(DATA_DIR / 'cosign-trusted-root.json'),
                        '--bundle',
                        str(temporary_bundle),
                        str(output),
                    ],
                    check=True,
                    env=self.env,
                )
                if not temporary_bundle.is_file() or temporary_bundle.stat().st_size == 0:
                    raise SbomException(f'Cosign did not produce a signature bundle for {output}')
                temporary_bundle.replace(bundle)
        except (OSError, subprocess.CalledProcessError) as ex:
            raise SbomException(
                f'Cosign signing failed for {output}: {ex}. No signature bundle was produced.'
            ) from ex
        log.inf(f'Signature bundle: {bundle}')
