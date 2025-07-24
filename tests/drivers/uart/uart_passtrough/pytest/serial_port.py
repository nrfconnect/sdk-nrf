# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause


import logging
import time

import serial

logger = logging.getLogger("serial_port")
logger.setLevel(logging.INFO)


class SerialPort:
    """
    Class for handling serial port
    Default settings:
    8 data bits, 1 stop bit, no parity
    """

    def __init__(self, serial_port: str, baudrate: int, timeout: int = 15):
        """
        :param serial_port: full name of the serial device port
        :param baudrate: buadrate [bps]
        :param timeout: read/write timeout [s]
        """
        self.port_name = serial_port
        self.baud_rate = baudrate
        self._timeout = timeout
        self.serial_port = self._instanitate_serial_port()

    def _instanitate_serial_port(self) -> serial.Serial:
        """
        Auxiliary method for handling serial port handler object instantiation
        """
        try:
            port = serial.Serial(
                port=self.port_name, baudrate=self.baud_rate, timeout=self._timeout, rtscts=True
            )
            port.set_low_latency_mode(True)
            return port
        except serial.SerialException as exc:
            logger.error(f"Opening '{self.port_name}' failed with error: {exc}!")
            raise exc

    def close(
        self,
    ):
        """
        Close serial port
        """
        if self.serial_port:
            logger.debug("Closing serial port connection")
            self.serial_port.close()
            logger.debug("Serial port connection closed")
            self.serial_port = None

    def open(self) -> None:
        """
        Open serial port
        """
        if not self.serial_port.is_open:
            logger.debug(f"Opening: '{self}'")
            self.serial_port.open()
            self.serial_port.reset_input_buffer()
            self.serial_port.reset_output_buffer()

    def send(
        self, message: str, get_response: bool = False, add_line_termination: bool = False
    ) -> str:
        """
        Send one message (and optionally receive response)
        """
        response: str = ""
        if self.serial_port.is_open is False:
            raise IOError("Serial port is closed")

        try:
            time.sleep(0.25)
            self.serial_port.write_timeout = self._timeout
            logger.debug(f"[{self.serial_port.port}] Serial --> {message}")
            line_termination: str = "\n" if add_line_termination else ""
            self.serial_port.write((message + line_termination).encode())
            if get_response:
                start = time.time()
                while (self.serial_port.in_waiting == 0) and (time.time() - start < self._timeout):
                    # no data in buffer yet
                    time.sleep(0.1)
                while (self.serial_port.in_waiting > 0) and (time.time() - start < self._timeout):
                    time.sleep(0.1)
                    response += self.serial_port.read_all().decode()
                logger.debug(f"Serial <-- {response}")
            return response

        except serial.SerialTimeoutException as exc:
            logger.error(f"Serial port read timeout: {exc}")

        except serial.SerialException as exc:
            logger.error(f"Serial port read error: {exc}")

        except UnicodeDecodeError as exc:
            logger.error(f"Decoding error: {exc}")

        return response.rstrip("\n")
