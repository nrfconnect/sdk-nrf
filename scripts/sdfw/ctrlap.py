#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Module exposing CTRLAP functionality
"""

from argparse import ArgumentParser
from enum import IntEnum
from time import sleep, time

from jlink_connector import JLinkConnector


class CtrlApReg(IntEnum):
    """Access Port register enumerations."""

    READY = 0x4
    TXDATA = 0x10
    RXDATA = 0x18
    RXSTATUS = 0x1C
    TXSTATUS = 0x14


class ReadyStatus(IntEnum):
    """Enum for READY status CTRLAP register"""

    READY = 0
    NOT_READY = 1


class MailboxStatus(IntEnum):
    """Enum for mailbox Rx/Tx status CTRLAP register"""

    NO_DATA_PENDING = 0
    DATA_PENDING = 1


class Ctrlap:
    """Class for performing CTRLAP operations"""

    def __init__(self, **kwargs) -> None:
        """
        :param kwargs: dict with wait_time and timeout.
        Use 'add_arguments(...)' function from this class to add required arguments to
        the argument parser.
        """
        self.connector = JLinkConnector(**kwargs)
        self.wait_time = kwargs["wait_time"]
        self.timeout = kwargs["timeout"]

    def wait_for_ready(self) -> None:
        """Wait until the CTRLAP READY register is ready.

        :raises Exception: If waiting times out.
        """

        self._block_on_ctrlap_status(CtrlApReg.READY, ReadyStatus.NOT_READY)

    def read(self) -> bytes:
        """
        Receive a word from CTRLAP by reading the RX register. Will block until there is
        data pending.

        :return: The value of CTRLAP.RX
        """

        self._block_on_ctrlap_status(CtrlApReg.RXSTATUS, MailboxStatus.NO_DATA_PENDING)
        word = self.connector.access_port_read(CtrlApReg.RXDATA)

        return word.to_bytes(4, "little")

    def write(self, word: bytes) -> None:
        """
        Write a word to CTRLAP, block until the data has been read.

        :param word: Word to write
        """

        val = int.from_bytes(word, byteorder="little")
        self.connector.access_port_write(CtrlApReg.TXDATA, val)
        self._block_on_ctrlap_status(CtrlApReg.TXSTATUS, MailboxStatus.DATA_PENDING)

    def _block_on_ctrlap_status(self, reg: CtrlApReg, status: IntEnum) -> None:
        """
        Block while waiting for another status

        :param reg: Access port register to check status of
        :param status: Block while the value of the register equals this value.
        :raises RuntimeError: if operation times out.
        """

        start = time()
        while (self.connector.access_port_read(reg)) == status:
            if (time() - start) >= self.timeout:
                raise RuntimeError("Timed out when waiting for CTRLAP status")
            sleep(self.wait_time)

    @classmethod
    def add_arguments(cls, parser: ArgumentParser) -> None:
        """
        Append command line options needed by this class.
        """

        group = parser.add_argument_group(
            title="CTRL-AP connection parameters",
            description="Use these parameters for communicating with CTRL-AP",
        )
        group.add_argument(
            "--timeout",
            type=float,
            default=100,
            help="Number of seconds to wait for a CTRL-AP register value before timing out",
        )

        group.add_argument(
            "--wait-time",
            type=float,
            default=0.1,
            help="Number of seconds to wait between reading the CTRL-AP registers while waiting for a given status",
        )

        JLinkConnector.add_arguments(parser)
