#!/usr/bin/env python3
#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from exceptions import ATCommandError, NoATClientException

ERR_CODE_TO_MSG = {
    513: 'Not found',
    514: 'Not allowed',
    515: 'Memory full',
    518: 'Not allowed in active state',
    519: 'Already exists',
    523: 'Key generation failed',
}

class ATClient():
    def __init__(self, serial_dev):
        if serial_dev is None:
            raise RuntimeError('Serial device is None')

        self.device = serial_dev

    def __at_command_ok(self, cmd):
        """Send AT command and return a bool whether the modem response was OK/ERROR."""
        try:
            self.at_command(cmd)
        except ATCommandError as err:
            print(err)
            return False
        return True

    def __response_line(self):
        """Read a single line from device and return a decoded and trimmed string."""
        line = self.device.readline()
        if line == b'':
            raise TimeoutError
        return line.decode('utf-8').strip()

    def __read_response(self, cmd):
        """Read full response for a command until it reaches OK or an error."""
        response = []
        while True:
            line = self.__response_line()

            if line == 'OK':
                break
            elif line == 'ERROR':
                raise ATCommandError(f'Error returned for AT command: {cmd}')
            elif line.startswith('+CME ERROR'):
                code = line.replace('+CME ERROR: ', '')
                msg = (
                    f'Error returned for AT command: {cmd}\n'
                    f'Error code: {code}\n'
                    f'Error message: {ERR_CODE_TO_MSG[int(code)]}'
                )
                raise ATCommandError(msg)
            else:
                response.append(line)

        return response

    def connect(self, dev, baudrate = 115200, timeout = 1):
        """Open a serial connection to the serial device"""

        self.device.port = dev
        self.device.baudrate = baudrate
        self.device.timeout = timeout
        self.device.open()

    def verify(self):
        """Check if modem responds to 'AT'

        Raises NoATClientException if unsuccessful after 3 attempts.
        """
        retries = 3
        try:
            while not self.__at_command_ok('AT'):
                retries = retries - 1
                if retries == 0:
                    raise NoATClientException
        except TimeoutError:
            raise NoATClientException
        return True

    def enable_error_codes(self):
        """Change error responses to include error code"""
        if not self.__at_command_ok('AT+CMEE=1'):
            print('Failed to enable error notifications')

    def at_command(self, cmd):
        """Send AT command to modem and return the response as a list of response lines.

        Raises ATCommandError if the modem responds with an error.
        """

        if not self.device.is_open:
            raise ConnectionError('Serial device not connected')

        self.device.reset_input_buffer()
        self.device.write((cmd + '\r\n').encode('utf-8'))
        return self.__read_response(cmd)
