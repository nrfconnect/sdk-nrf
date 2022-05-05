#!/usr/bin/env python3
#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""AT command handling"""

import sys
import logging
import serial

class ATclient:
    """Send and receive AT commands to modem through serial port."""
    def __init__(self, serial_path):
        self.serial = serial.Serial(serial_path, 115200, timeout=0.1)
        if not self.is_ready():
            logging.error("AT interface not responding")
            sys.exit(1)

    def is_ready(self):
        """\
        Check if AT client is passing our AT commands
        to the modem, and the modem is responding
        """
        self.serial.write(b'\r')
        self.serial.readlines()  # empty buffers
        for _ in range(3):
            if self.at_cmd('AT', timeout=1) is None:
                return False
        logging.debug("AT interface ready")
        return True

    def at_cmd(self, at_str, timeout=100):
        """\
        Send AT command to the modem and parse the response
        if OK is found from response, that line is dropped
        """
        if isinstance(at_str, str):
            at_str = at_str.encode()
        self.serial.write(at_str)
        logging.debug(at_str.decode())
        if not b'\r' in at_str:
            self.serial.write(b'\r')
        resp = []
        while True:
            line = self.serial.readline()
            if len(line) != 0:
                line = line.decode()
                logging.debug(line.strip())
                resp.append(line)
                if 'OK\r\n' in line or 'ERROR' in line:
                    break
            else:
                timeout -= 1
                if timeout == 0:
                    break
        try:
            if 'OK\r\n' not in resp[-1]:
                return None
        except IndexError:
            return None
        resp.pop()
        return ''.join(resp).strip()
