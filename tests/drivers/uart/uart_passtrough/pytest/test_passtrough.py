# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import logging
import time
from collections.abc import Callable

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
        line_termination = b"\n"
    else:
        line_termination = "\n"
    write_handler(line_termination)


def test_uart_passtrough(dut: DeviceAdapter):
    """
    Verify the UART darta forwarding capability.
    Send message from 'console' to 'passtrough' UART and verify it,
    then send message from 'passtrough' UART to 'console 'UART' and verify it.
    """

    dut.readlines_until(regex="Ready", print_output=True, timeout=2)
    dut.disconnect()
    console_serial_port: SerialPort = SerialPort(
        serial_port=dut.device_config.serial, baudrate=dut.device_config.baud
    )

    if "if00" in dut.device_config.serial:
        second_serial_port_name = dut.device_config.serial.replace("if00", "if02")
    elif "if02" in dut.device_config.serial:
        second_serial_port_name = dut.device_config.serial.replace("if02", "if00")

    passtrough_serial_port: SerialPort = SerialPort(
        serial_port=second_serial_port_name, baudrate=dut.device_config.baud
    )

    first_message: str = "TEST1: console->passtrough"
    second_message: str = "TEST2: passtrough->console"

    console_serial_port.open()
    passtrough_serial_port.open()
    send_with_dead_time_between_characters(
        console_serial_port.send, first_message, DEAD_TIME_S
    )
    first_search_result: bool = passtrough_serial_port.read_data_until_matched(first_message)
    send_with_dead_time_between_characters(
        passtrough_serial_port.send, second_message, DEAD_TIME_S
    )
    second_search_result: bool = console_serial_port.read_data_until_matched(second_message)
    passtrough_serial_port.close()
    console_serial_port.close()

    assert first_search_result is True, f"{first_message} not found"
    assert second_search_result is True, f"{second_message} not found"
