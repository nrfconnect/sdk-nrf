# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import logging

logger = logging.getLogger("uart_passtrough")
logger.setLevel(logging.DEBUG)

from twister_harness import DeviceAdapter
from serial_port import SerialPort


def test_uart_passtrough(dut: DeviceAdapter):
    """
    Verify the UART darta forwarding capability.
    Send message from 'passtrough' UART to 'console 'UART' and verify it,
    then send message from 'console' to 'passtrough' UART and verify it.
    """

    if "if00" in dut.device_config.serial:
        second_serial_port_name = dut.device_config.serial.replace("if00", "if02")
    elif "if02" in dut.device_config.serial:
        second_serial_port_name = dut.device_config.serial.replace("if02", "if00")

    second_serial_port: SerialPort = SerialPort(
        serial_port=second_serial_port_name, baudrate=dut.device_config.baud
    )

    dut.readlines_until(regex="Ready", print_output=True, timeout=2)
    second_serial_port.open()
    second_serial_port.send("con->pass")
    dut.readlines_until(regex="con->pass", print_output=True, timeout=2)
    dut.write("pass<-con".encode())
    second_port_data: str = second_serial_port.send("", get_response=True)
    assert "pass<-con" in second_port_data
    second_serial_port.close()
