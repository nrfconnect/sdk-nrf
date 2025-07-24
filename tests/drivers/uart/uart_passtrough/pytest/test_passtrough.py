# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import logging
import time
from typing import Callable

from serial_port import SerialPort
from twister_harness import DeviceAdapter

logger = logging.getLogger("uart_passtrough")
logger.setLevel(logging.DEBUG)

DEAD_TIME_S = 100e-6


def send_with_dead_time_between_characters(
    write_handler: Callable, message: str, dead_time_s: float, apply_encoding: bool = False
):
    """
    Send message to serial
    and insert given dead time after every character
    Send line termination at the end
    """
    for character in message:
        if apply_encoding:
            character = character.encode()
        write_handler(character)
        time.sleep(dead_time_s)
    if apply_encoding:
        line_termination = "\n".encode()
    else:
        line_termination = "\n"
    write_handler(line_termination)


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
    send_with_dead_time_between_characters(
        second_serial_port.send, "TEST1: console->passtrough", DEAD_TIME_S
    )
    dut.readlines_until(regex="TEST1: console->passtrough", print_output=True, timeout=2)
    send_with_dead_time_between_characters(
        dut.write, "TEST2: passtrough->console", DEAD_TIME_S, apply_encoding=True
    )
    second_port_data: str = second_serial_port.send("", get_response=True)
    logger.debug(second_port_data)
    assert "TEST2: passtrough->console" in second_port_data
    second_serial_port.close()
