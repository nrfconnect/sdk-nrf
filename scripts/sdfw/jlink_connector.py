#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Module exposing a JLink connection
"""
from argparse import ArgumentParser

import pylink

AP_ID_CTRLAP = 4
AP_BANK_1 = 1
# Ref ARM ADIv5 B2.2.9 "SELECT, AP Select register"
DP_SELECT_APSEL_OFFSET = 24
DP_SELECT_APBANKSEL_OFFSET = 4
DP_SELECT_CTRLAP_BANK_1 = (AP_ID_CTRLAP << DP_SELECT_APSEL_OFFSET) | (
    AP_BANK_1 << DP_SELECT_APBANKSEL_OFFSET
)
DP_SELECT_ADDR = 0x8


class JLinkConnector:
    """Connect and maintain a JLink connection"""

    def __init__(self, **kwargs) -> None:
        """
        :param kwargs: dict with serial, hostname and port
        Use the `add_arguments(...)` function from this class to add those to
        your CLI's argument list.
        """

        self.params = kwargs
        self._jlink = pylink.JLink()

    def connect(self) -> None:
        """
        Open the JLink DLL, configure coresight operations and select the CTRL-AP.
        """
        opts = dict()

        if self.params["hostname"]:
            opts["ip_addr"] = f'{self.params["hostname"]}:{self.params["port"]}'
        if self.params["serial"]:
            opts["serial_no"] = self.params["serial"]

        self._jlink.open(**opts)
        self._jlink.set_tif(pylink.enums.JLinkInterfaces.SWD)
        self._jlink.coresight_configure()

        # Select CTRL-AP bank 1 with the Debug Port
        self._jlink.coresight_write(
            reg=self._get_index_from_address(DP_SELECT_ADDR),
            data=DP_SELECT_CTRLAP_BANK_1,
            ap=False,
        )

    def disconnect(self) -> None:
        """
        Disconnect from JLink and close the API.
        """
        self._jlink.close()

    def access_port_read(self, register_address: int) -> int:
        """
        Read the value of an access port register.

        :return: The value read from the given register index.
        """

        val = self._jlink.coresight_read(
            reg=self._get_index_from_address(register_address)
        )
        return val

    def access_port_write(self, register_address: int, value: int) -> None:
        self._jlink.coresight_write(
            reg=self._get_index_from_address(register_address), data=value
        )

    @staticmethod
    def add_arguments(parser: ArgumentParser) -> None:
        """
        Append command line options needed by this class.
        """

        group = parser.add_argument_group(
            title="JLink connection parameters",
            description="Use these parameters for connecting to JLink",
        )

        group.add_argument(
            "--port",
            type=int,
            default=19020,
            help="Port to use when connecting to J-Link emulator host",
        )

        group.add_argument("--hostname", type=str, help="J-Link emulator hostname IP")

        group.add_argument("--serial", type=int, help="J-Link emulator serial number")

    @staticmethod
    def _get_index_from_address(register_address: int) -> int:
        return register_address // 4
