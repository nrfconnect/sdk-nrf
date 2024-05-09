#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
SDFW ADAC command structures.
"""

from __future__ import annotations

import dataclasses as dc
import struct
from enum import IntEnum


class SdfwAdacCmd(IntEnum):
    """ADAC command opcodes"""

    VERSION = 0xA300
    MEM_CFG = 0xA301
    REVERT = 0xA302
    RESET = 0xA303
    MEM_ERASE = 0xA304
    LCS_GET = 0xA305
    LCS_SET = 0xA306
    SSF = 0xA307
    PURGE = 0xA308


@dc.dataclass
class AdacRequest:
    """Generic ADAC request"""

    command: SdfwAdacCmd
    data: bytes = b""

    @property
    def data_count(self) -> int:
        return len(self.data)

    def to_bytes(self) -> bytes:
        header = struct.pack("<HHI", 0, self.command.value, self.data_count)
        req = header + self.data
        return req


class AdacStatus(IntEnum):
    """Enum for ADAC response statuses"""

    ADAC_SUCCESS = 0
    ADAC_FAILURE = 1
    ADAC_NEED_MORE_DATA = 2
    ADAC_UNSUPPORTED = 3
    ADAC_INVALID_COMMAND = 0x7FFF


@dc.dataclass
class AdacResponse:
    """Generic ADAC response"""

    status: AdacStatus
    data: bytes

    @property
    def data_count(self) -> int:
        return len(self.data)


@dc.dataclass
class Version:
    """Get version of vendor specific SDFW ADAC commands."""

    type: int

    def to_request(self) -> AdacRequest:
        return AdacRequest(
            command=SdfwAdacCmd.VERSION,
            data=struct.pack("<I", self.type),
        )


@dc.dataclass
class MemCfg:
    """Enable memory access for a local domain to a given memory region."""

    domain_id: int
    address: int
    length: int

    def to_request(self) -> AdacRequest:
        return AdacRequest(
            command=SdfwAdacCmd.MEM_CFG,
            data=struct.pack("<HHII", self.domain_id, 0, self.address, self.length),
        )


@dc.dataclass
class Revert:
    """Revert configuration applied through ADAC."""

    def to_request(self) -> AdacRequest:
        return AdacRequest(command=SdfwAdacCmd.REVERT)


@dc.dataclass
class Reset:
    """Reset the whole system or a specific local domain."""

    domain_id: int = 0
    mode: int = 0

    def to_request(self) -> AdacRequest:
        return AdacRequest(
            command=SdfwAdacCmd.RESET,
            data=struct.pack("<HBB", 0, self.domain_id, self.mode),
        )


@dc.dataclass
class MemErase:
    """Erase a memory region. Note that the word size is 16 bytes."""

    address: int
    num_words: int

    def to_request(self) -> AdacRequest:
        return AdacRequest(
            command=SdfwAdacCmd.MEM_ERASE,
            data=struct.pack("<II", self.address, self.num_words),
        )


@dc.dataclass
class LcsGet:
    """Get the LifeCycle State (LCS) of a local domain."""

    domain_id: int

    def to_request(self) -> AdacRequest:
        return AdacRequest(
            command=SdfwAdacCmd.LCS_GET, data=struct.pack("<I", self.domain_id)
        )


@dc.dataclass
class LcsSet:
    """Set the LifeCycle State (LCS) of a local domain."""

    domain_id: int
    current: int
    new: int

    def to_request(self) -> AdacRequest:
        return AdacRequest(
            command=SdfwAdacCmd.LCS_SET,
            data=struct.pack("<III", self.domain_id, self.current, self.new),
        )


@dc.dataclass
class Ssf:
    """Send an SSF request."""

    data: bytes

    def to_request(self) -> AdacRequest:
        return AdacRequest(
            command=SdfwAdacCmd.SSF,
            data=self.data,
        )


@dc.dataclass
class Purge:
    """Purge a given local domain."""

    domain_id: int

    def to_request(self) -> AdacRequest:
        return AdacRequest(
            command=SdfwAdacCmd.PURGE,
            data=struct.pack("<I", self.domain_id),
        )
