# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Utilities for parsing and manipulating MCUboot image headers and TLV areas."""

from __future__ import annotations

import logging
import struct
from dataclasses import astuple, dataclass, field
from pathlib import Path

from intelhex import IntelHex

logger = logging.getLogger(__name__)

TLV_INFO_SIZE = 4


class ImageUtilsException(Exception):
    """Exception raised for errors in MCUboot image utilities."""


@dataclass
class ImageHeader:
    """Class representing the header of an MCUboot image."""

    magic: int
    load_addr: int
    hdr_size: int
    protected_tlv_size: int
    img_size: int
    flags: int

    @classmethod
    def from_bytes(cls, data: bytes) -> "ImageHeader":
        """Create an ImageHeader instance from bytes."""
        # 5 * 4 bytes (uint32_t) + 2 * 2 bytes (uint16_t)
        return cls(*struct.unpack("<I I H H I I", data))

    def to_bytes(self) -> bytes:
        """Convert the ImageHeader instance to bytes."""
        return struct.pack("<I I H H I I", *astuple(self))


@dataclass
class TLV:
    """Class representing a TLV (Type-Length-Value) entry."""

    tlv_type: int
    tlv_len: int
    tlv_data: bytes
    tlv_off: int


@dataclass
class Image:
    """Class representing an MCUboot image."""

    image_file: Path
    header: ImageHeader | None = None
    ih: IntelHex | None = None
    tlvs_protected: list[TLV] = field(default_factory=list)
    tlvs: list[TLV] = field(default_factory=list)

    def parse(self):
        """Parse the image file and extract header and TLVs."""
        self.ih = IntelHex()
        self.ih.loadbin(self.image_file)
        self.header = ImageHeader.from_bytes(self.ih.gets(0, 20))
        self._parse_tlvs()

    def _parse_tlvs(self):
        """Parse TLV areas from the image."""
        tlv_off = self.header.hdr_size + self.header.img_size  # type: ignore
        if self.header.protected_tlv_size:  # type: ignore
            tlv_off = self._parse_tlv_area(tlv_off, self.tlvs_protected)
        tlv_off = self._parse_tlv_area(tlv_off, self.tlvs)

    def _parse_tlv_area(self, tlv_off: int, tlvs: list["TLV"]) -> int:
        """Parse a single TLV area and append TLVs to the list."""
        _, tlv_tot = struct.unpack("HH", self.ih.gets(tlv_off, TLV_INFO_SIZE))  # type: ignore
        tlv_end = tlv_off + tlv_tot
        tlv_off += TLV_INFO_SIZE
        while tlv_off < tlv_end:
            tlv_type, tlv_len = struct.unpack("HH", self.ih.gets(tlv_off, TLV_INFO_SIZE))  # type: ignore
            tlv_off += TLV_INFO_SIZE
            tlv_data = self.ih.gets(tlv_off, tlv_len)  # type: ignore
            tlvs.append(TLV(tlv_type, tlv_len, tlv_data, tlv_off))
            tlv_off += tlv_len
        return tlv_off


def copy_tlvs_areas(from_app: Path, to_app: Path, output_app: Path | None = None) -> Path:
    """Copy TLV area from one image to another."""
    from_img = Image(from_app)
    from_img.parse()
    to_img = Image(to_app)
    to_img.parse()
    assert from_img.header == to_img.header, "Header of both images must be identical"

    for tlv in from_img.tlvs_protected + from_img.tlvs:
        logger.debug(f"Copy protected TLV area of type 0x{tlv.tlv_type:x} at offset 0x{tlv.tlv_off:x}")
        to_img.ih.puts(tlv.tlv_off, tlv.tlv_data)  # type: ignore

    # Save modified image
    output_app = output_app or to_app
    to_img.ih.tobinfile(output_app)  # type: ignore
    return output_app


def change_byte_in_tlv_area(image_file: Path, tlv_type: int, output_app: Path | None = None) -> Path:
    """Modify the TLV area for given type."""
    img = Image(image_file)
    img.parse()
    for tlv in img.tlvs_protected + img.tlvs:
        if tlv.tlv_type == tlv_type:
            logger.debug(f"Found TLV type 0x{tlv_type:x} at offset 0x{tlv.tlv_off:x}")
            break
    else:
        raise ImageUtilsException(f"TLV type {tlv_type:x} not found")

    logger.debug(f"Modify the TLV data at offset 0x{tlv.tlv_off:x} for type 0x{tlv.tlv_type:x}")
    img.ih._buf[tlv.tlv_off] ^= 0xFF  # type: ignore

    # Save modified image
    if not output_app:
        output_app = image_file.parent / f"{image_file.stem}_tlv{tlv_type:x}{image_file.suffix}"
    img.ih.tobinfile(output_app)  # type: ignore
    return output_app
