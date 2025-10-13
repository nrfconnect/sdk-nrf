# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Tool for verifying LZMA2 compression and decompression of MCUboot images."""

from __future__ import annotations

import argparse
import logging
import sys
import tempfile
import textwrap
from pathlib import Path

from helpers import run_command
from intelhex import IntelHex
from mcuboot_image_utils import ImageHeader

logger = logging.getLogger(__name__)

EXPECTED_MAGIC = 0x96F3B83D
LZMA_HEADER_SIZE = 2
FLAG_LZMA2 = 0x400
FLAG_ARM_THUMB = 0x800


class CheckCompressionError(Exception):
    """Exception raised for errors in LZMA compression check."""


def check_lzma_compression(
    signed_bin: str | Path,
    unsigned_bin: str | Path,
    workdir: str | Path | None = None,
    padding: int = 0,
) -> None:
    """Check if the image is compressed with LZMA2.

    Verify if decompressed data is identical as before compression.
    """
    logger.debug(f"check_lzma_compression: {signed_bin=}, {unsigned_bin=}")
    workdir = Path(workdir) if workdir else Path.cwd()
    ih_signed = IntelHex()
    ih_signed.loadbin(signed_bin)

    logger.info("Check image header of signed image")
    header = ImageHeader.from_bytes(ih_signed.gets(0, 20))
    logger.debug(header)
    if header.magic != EXPECTED_MAGIC:
        raise CheckCompressionError("Invalid magic value")
    if header.flags & FLAG_LZMA2 == 0:
        raise CheckCompressionError("Not LZMA2 compression")

    logger.info("Extracting LZMA stream from signed image")
    lzma_stream_size = header.img_size - LZMA_HEADER_SIZE
    lzma_stream_offset = header.hdr_size + LZMA_HEADER_SIZE
    ih_lzma = IntelHex()
    ih_lzma.frombytes(ih_signed.gets(lzma_stream_offset, lzma_stream_size))
    ih_lzma.tobinfile(workdir / "stream.lzma")

    logger.info("Decompressing LZMA stream")
    unlzma_cmd = [
        "unlzma",
        "--armthumb" if header.flags & FLAG_ARM_THUMB else "",
        "--lzma2",
        "--format=raw",
        "--suffix=.lzma",
        str(workdir / "stream.lzma"),
    ]
    run_command(unlzma_cmd)

    logger.info("Verifying decompressed data")
    ih_unsigned = IntelHex()
    ih_unsigned.loadbin(unsigned_bin)
    if padding:
        # For platforms that don't use partition manager, the original image is offset by padding
        # which is later filled with the header. For comparison, this padding needs to be removed.
        ih_unsigned_without_padding = IntelHex()
        ih_unsigned_without_padding.frombytes(
            ih_unsigned.gets(header.hdr_size, ih_unsigned.maxaddr() - header.hdr_size + 1)  # type: ignore
        )
        ih_unsigned = ih_unsigned_without_padding
    ih_unlzma = IntelHex()
    ih_unlzma.loadbin(workdir / "stream")

    if ih_unsigned.maxaddr() != ih_unlzma.maxaddr():
        raise CheckCompressionError("Decompressed data length is not identical as before compression")
    if ih_unsigned.tobinarray() != ih_unlzma.tobinarray():
        raise CheckCompressionError("Decompressed data is not identical as before compression")
    logger.info("Decompressed data is identical as before compression")


def create_parser() -> argparse.ArgumentParser:
    """Create an argument parser for the LZMA compression check tool."""
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
        description="LZMA test",
        epilog=(
            textwrap.dedent("""
            This tool finds and extracts compressed stream, decompresses it and verifies if
            decompressed one is identical as before compression.
        """)
        ),
    )
    parser.add_argument("signed_bin", metavar="path", help="Path to the signed binary file")
    parser.add_argument("unsigned_bin", metavar="path", help="Path to the unsigned binary file")

    parser.add_argument(
        "--padding",
        type=int,
        default=0,
        help="Padding value for platforms that don't use partition manager (default: 0)",
    )
    parser.add_argument(
        "-ll", "--log-level", type=str.upper, default="INFO", choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"]
    )
    return parser


def main():
    """Run the LZMA compression check tool."""
    parser = create_parser()
    args = parser.parse_args()
    logging.basicConfig(level=args.log_level, format="%(levelname)-8s:  %(message)s")

    try:
        with tempfile.TemporaryDirectory() as tmpdir:
            check_lzma_compression(args.signed_bin, args.unsigned_bin, tmpdir, args.padding)
    except CheckCompressionError as e:
        logger.error(e)
        sys.exit(1)


if __name__ == "__main__":
    main()
