#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''Install the cosign release configured in data/cosign-release.json.'''

import argparse
import hashlib
import json
import os
import platform
import shutil
import sys
from pathlib import Path
from tempfile import TemporaryDirectory
from urllib.request import urlopen

RELEASE_FILE = Path(__file__).resolve().parents[1] / 'data' / 'cosign-release.json'


class CosignError(Exception):
    '''An installation or executable lookup error.'''


def release_asset() -> str:
    '''Select the official binary for the current OS and architecture.'''
    system = platform.system().lower()
    machine = platform.machine().lower()
    arch = {'x86_64': 'amd64', 'aarch64': 'arm64'}.get(machine, machine)
    suffix = '.exe' if system == 'windows' else ''
    return f'cosign-{system}-{arch}{suffix}'


def default_install_dir(version: str) -> Path:
    '''Return a persistent per-user directory, independent of the west environment.'''
    if platform.system() == 'Windows':
        cache = Path(os.environ.get('LOCALAPPDATA', Path.home() / 'AppData' / 'Local'))
    elif platform.system() == 'Darwin':
        cache = Path.home() / 'Library' / 'Caches'
    else:
        cache = Path(os.environ.get('XDG_CACHE_HOME', Path.home() / '.cache'))
    target = release_asset().removeprefix('cosign-').removesuffix('.exe')
    return cache / 'west-ncs-sbom' / 'cosign' / version / target


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def install_cosign(install_dir: 'Path|None' = None) -> Path:
    '''Download once, verify before installation, and reuse a matching installation.'''
    asset = release_asset()
    try:
        release = json.loads(RELEASE_FILE.read_text(encoding='utf-8'))
        version, checksums = release['version'], release['sha256']
        if asset not in checksums:
            raise CosignError(f'No checksum for {asset} in {RELEASE_FILE}; use --cosign PATH')
        directory = Path(install_dir) if install_dir is not None else default_install_dir(version)
        binary = directory.resolve() / ('cosign.exe' if asset.endswith('.exe') else 'cosign')
        if binary.exists():
            if _sha256(binary) != checksums[asset]:
                raise CosignError(f'Checksum mismatch for existing cosign: {binary}')
            return binary

        binary.parent.mkdir(parents=True, exist_ok=True)
        url = f'https://github.com/sigstore/cosign/releases/download/v{version}/{asset}'
        print(f'Installing cosign {version} to {binary}', file=sys.stderr)
        with TemporaryDirectory(dir=binary.parent, prefix='.cosign-') as temporary_dir:
            temporary = Path(temporary_dir) / binary.name
            with urlopen(url, timeout=60) as response, temporary.open('wb') as target:
                shutil.copyfileobj(response, target, length=1024 * 1024)
            if _sha256(temporary) != checksums[asset]:
                raise CosignError(f'Checksum mismatch for downloaded {asset}')
            temporary.chmod(0o755)
            temporary.replace(binary)
        return binary
    except (OSError, ValueError, KeyError, TypeError) as ex:
        raise CosignError(f'Cannot install cosign: {ex}') from ex


def ensure_cosign(executable: 'str|None' = None) -> str:
    '''Use an explicit executable, PATH, or the managed installation, in that order.'''
    binary = shutil.which(executable or 'cosign')
    if binary is not None:
        return binary
    if executable is not None:
        raise CosignError(f'Cannot find cosign executable: {executable}')
    return str(install_cosign())


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument(
        '--install-dir',
        type=Path,
        help='Destination directory; defaults to a persistent per-user cache.',
    )
    options = parser.parse_args()
    try:
        print(install_cosign(options.install_dir))
    except CosignError as ex:
        parser.exit(1, f'Error: {ex}\n')


if __name__ == '__main__':
    main()
