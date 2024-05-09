#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Module exposing a JLink connection
"""

from argparse import ArgumentParser

from pynrfjprog import LowLevel
from pynrfjprog.Parameters import CoProcessor, DeviceFamily


class JLinkConnector:
    """Connect and maintain a JLink connection"""

    def __init__(self, auto_connect: bool = True, **kwargs) -> None:
        """
        :param auto_connect: automatically connect to JLink if this is set.
        Otherwise, the `connect(...)` function must be used.
        :param kwargs: dict with serial, hostname, speed and port
        Use the `add_arguments(...)` function from this class to add those to
        your CLI's argument list.
        """

        self.params = kwargs
        self.device = DeviceFamily.NRF54H
        self.api = LowLevel.API(self.device)
        if auto_connect:
            self.connect()

    def connect(self) -> None:
        """
        Connect to the JLink
        """
        if not self.api.is_open():
            self.api.open()

        if self.params["hostname"] and self.params["serial"]:
            self.api.connect_to_emu_with_ip(
                hostname=self.params["hostname"],
                serial_number=self.params["serial"],
                port=self.params["port"],
                jlink_speed_khz=self.params["speed"],
            )
        elif self.params["hostname"]:
            self.api.connect_to_emu_with_ip(
                hostname=self.params["hostname"],
                port=self.params["port"],
                jlink_speed_khz=self.params["speed"],
            )
        elif self.params["serial"]:
            self.api.connect_to_emu_with_snr(
                serial_number=self.params["serial"],
                jlink_speed_khz=self.params["speed"],
            )
        else:
            self.api.connect_to_emu_without_snr(jlink_speed_khz=self.params["speed"])

        self.api.select_coprocessor(
            LowLevel.Parameters.CoProcessor[self.params["coprocessor"].upper()]
        )

    def disconnect(self) -> None:
        """
        Disconnect from JLink and close the API.
        """
        self.api.disconnect_from_emu()
        self.api.close()

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
            "--speed", type=int, default=4000, help="J-Link emulator speed in kHz"
        )

        group.add_argument(
            "--port",
            type=int,
            default=19020,
            help="Port to use when connecting to J-Link emulator host",
        )

        group.add_argument("--hostname", type=str, help="J-Link emulator hostname IP")

        group.add_argument("--serial", type=int, help="J-Link emulator serial number")

        group.add_argument(
            "--coprocessor",
            type=str,
            help="Coprocessor (AP) to connect to",
            default=CoProcessor.CP_SECURE.name.lower(),
            choices=[i.name.lower() for i in CoProcessor],
        )
