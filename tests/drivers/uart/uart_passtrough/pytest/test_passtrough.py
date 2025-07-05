# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import logging
import pytest
import time

logger = logging.getLogger("uart_passtrough")
logger.setLevel(logging.DEBUG)

from twister_harness import DeviceAdapter
from serial_port import SerialPort


def test_uart_passtrough(dut: DeviceAdapter):
    if "if00" in dut.device_config.serial:
        second_serial_port_name = dut.device_config.serial.replace("if00", "if02")
    elif "if02" in dut.device_config.serial:
        second_serial_port_name = dut.device_config.serial.replace("if02", "if00")

    second_serial_port: SerialPort = SerialPort(
        serial_port=second_serial_port_name, baudrate=dut.device_config.serial.baud
    )

    second_serial_port.open()
    second_serial_port.send("test")

    dut.readlines()
    # received_lines: str = " ".join(dut.readlines())
    # logger.info(received_lines)

    second_serial_port.close()
