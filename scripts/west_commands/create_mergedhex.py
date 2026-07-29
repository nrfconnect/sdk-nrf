# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

'''Merge the hex files that "west flash" programs into a single distributable hex file.'''

import os
import sys
from argparse import ArgumentParser, Namespace
from pathlib import Path
from typing import Any

import yaml
from west.commands import CommandError, WestCommand


def _import_zephyr_build_helpers(zephyr_base: str):
    # build_helpers lives in Zephyr's west command directory and pulls in
    # zcmake and pylib/build_helpers/domains.py from there itself.
    scripts = os.path.join(zephyr_base, 'scripts', 'west_commands')
    if scripts not in sys.path:
        sys.path.insert(0, scripts)
    import build_helpers

    return build_helpers


class CreateMergedHex(WestCommand):
    def __init__(self) -> None:
        super().__init__(
            name='create-mergedhex',
            help='merge the hex files flashed by "west flash" into one hex file',
            description='''\
Merge the images that "west flash" would program into a single hex file that
can be distributed and programmed in one operation.

The set of images and their order is taken from the same places "west flash"
reads them from: domains.yaml in the build directory (flash_order), and the
hex_file entry of each image's zephyr/runners.yaml. Non-sysbuild builds
without a domains.yaml are handled as a single image.

The hex files are used as they are found in the build directory; this command
does not rebuild them.''',
        )

    def do_add_parser(self, parser_adder: Any) -> ArgumentParser:
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )

        parser.add_argument(
            '-d',
            '--build-dir',
            help='build directory to read images from (default: same heuristic as west flash)',
        )
        parser.add_argument(
            '-o',
            '--output',
            help='output file (default: <build-dir>/merged.hex, '
            'or <build-dir>/merged.bin with --bin)',
        )
        parser.add_argument(
            '--domain',
            action='append',
            dest='domains',
            metavar='DOMAIN',
            help='only merge this domain/image; may be given more than once. '
            'The default is every domain in flash order.',
        )
        parser.add_argument(
            '--overlap',
            choices=('error', 'ignore', 'replace'),
            default='error',
            help='what to do when two images overlap (default: error)',
        )
        parser.add_argument(
            '--bin',
            action='store_true',
            help='write a binary image instead of a hex file. Note that gaps '
            'between images are padded with 0xff and that the load address is lost.',
        )
        parser.add_argument(
            '-l',
            '--list',
            action='store_true',
            help='only list the images that would be merged, then exit',
        )

        return parser

    def do_run(self, args: Namespace, _: list[str]) -> None:
        build_dir = self._find_build_dir(args)
        images = self._collect_images(build_dir, args.domains)

        for name, hex_file in images:
            self.inf(f'-- {self.name}: {name}: {hex_file}')

        if args.list:
            return

        output = Path(args.output) if args.output else \
            Path(build_dir) / ('merged.bin' if args.bin else 'merged.hex')

        self._merge(images, output, args.overlap, args.bin)

    def _find_build_dir(self, args: Namespace) -> str:
        zephyr = self.manifest.get_projects(['zephyr'], only_cloned=True)
        if not zephyr:
            raise CommandError('the zephyr project is not cloned in this workspace')
        self._build_helpers = _import_zephyr_build_helpers(zephyr[0].abspath)

        build_dir = self._build_helpers.find_build_dir(args.build_dir)
        if not self._build_helpers.is_zephyr_build(build_dir):
            raise CommandError(
                f'{build_dir} is not a Zephyr build directory; '
                'run this from a build directory or pass --build-dir'
            )
        self.dbg(f'build directory: {build_dir}')
        return build_dir

    def _collect_images(
        self, build_dir: str, names: list[str] | None
    ) -> list[tuple[str, Path]]:
        '''Return [(domain name, hex file)] in the order west flash programs them.'''
        domains = self._build_helpers.load_domains(build_dir)
        # Without --domain, mirror west flash: every domain, in flash order.
        selected = domains.get_domains(names, default_flash_order=names is None)

        images = []
        for domain in selected:
            images.append((domain.name, self._hex_file(domain)))
        if not images:
            raise CommandError(f'no images to merge found in {build_dir}')
        return images

    def _hex_file(self, domain: Any) -> Path:
        runners_yaml = Path(domain.build_dir) / 'zephyr' / 'runners.yaml'
        if not runners_yaml.is_file():
            raise CommandError(
                f'no runners.yaml found for domain "{domain.name}" '
                f'({runners_yaml}); it cannot be merged'
            )

        with open(runners_yaml, encoding='utf-8') as f:
            content = yaml.safe_load(f.read())

        hex_file = (content.get('config') or {}).get('hex_file')
        if not hex_file:
            raise CommandError(f'no hex_file configured in {runners_yaml}')

        # Paths in runners.yaml are relative to the directory containing it.
        resolved = Path(runners_yaml.parent / hex_file)
        if not resolved.is_file():
            raise CommandError(
                f'hex file for domain "{domain.name}" does not exist: {resolved}. '
                'Has the build completed?'
            )
        return resolved

    def _merge(
        self, images: list[tuple[str, Path]], output: Path, overlap: str, as_bin: bool
    ) -> None:
        try:
            from intelhex import AddressOverlapError, IntelHex
        except ImportError as e:
            raise CommandError(
                'the intelhex Python package is required; '
                'install it with "pip install intelhex"'
            ) from e

        merged = IntelHex()
        for name, hex_file in images:
            image = IntelHex(os.fspath(hex_file))
            # objcopy emits a start address record that is not meaningful here
            # and would conflict when merging.
            image.start_addr = None
            try:
                merged.merge(image, overlap=overlap)
            except AddressOverlapError as e:
                raise CommandError(
                    f'{name} ({hex_file}) overlaps an already merged image: {e}. '
                    'Use --overlap to decide how to handle this.'
                ) from e

        output.parent.mkdir(parents=True, exist_ok=True)
        merged.tofile(os.fspath(output), format='bin' if as_bin else 'hex')

        total = 0
        for start, end in merged.segments():
            self.inf(f'-- {self.name}: 0x{start:08x}-0x{end - 1:08x} ({end - start} bytes)')
            total += end - start
        self.inf(f'-- {self.name}: wrote {output} ({total} bytes of image data)')
