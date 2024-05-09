#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Module for communicating with the SDFW using the ADAC protocol over the CTRLAP interface
"""

from __future__ import annotations

import struct
from typing import List

from ctrlap import Ctrlap
from sdfw_adac_cmd import AdacRequest, AdacResponse, AdacStatus

BYTES_PER_WORD = 4


class Adac:
    """SDFW ADAC connector."""

    def __init__(
        self,
        ctrlap: Ctrlap,
    ) -> None:
        """
        :param ctrlap: Used for performing ctrlap operations
        """
        self.ctrlap = ctrlap

    def request(self, req: AdacRequest) -> AdacResponse:
        """
        Issue an ADAC request and read the response.

        :param req: The ADAC request to execute.
        :returns: ADAC response.
        """
        self.ctrlap.wait_for_ready()
        self._write_request(req)

        self.ctrlap.wait_for_ready()
        rsp = self._read_response()

        return rsp

    def _write_request(self, req: AdacRequest) -> None:
        """
        Write an ADAC request to CTRL-AP

        :param req: ADAC request.
        """
        for word in _to_word_chunks(req.to_bytes()):
            self.ctrlap.write(word)

    def _read_response(self) -> AdacResponse:
        """
        Read a whole ADAC response over CTRL-AP

        :return: ADAC response.
        """
        _reserved, status = struct.unpack("<HH", self.ctrlap.read())
        (data_count,) = struct.unpack("<I", self.ctrlap.read())

        data = bytearray()
        for _ in range(data_count // BYTES_PER_WORD):
            data.extend(self.ctrlap.read())

        status_enum = AdacStatus(status)

        return AdacResponse(status=status_enum, data=bytes(data))


def _to_word_chunks(data: bytes) -> List[bytes]:
    num_align_bytes = (4 - (len(data) % 4)) % 4
    data_word_aligned = data + bytes(num_align_bytes)

    return [data_word_aligned[i: i + BYTES_PER_WORD] for i in range(0, len(data_word_aligned), BYTES_PER_WORD)]
