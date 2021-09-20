#!/usr/bin/env python3
#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import logging
import serial
import re


class ATclient:
    def __init__(self, serial):
        self.serial = serial

    def is_ready(self):
        """\
        Check if AT client is passing our AT commands
        to the modem, and the modem is responding
        """
        self.serial.write(b'\r')
        self.serial.readlines()  # empty buffers
        for i in range(3):
            if self.at_cmd('AT', timeout=1) is None:
                return False
        return True

    def at_cmd(self, at, timeout=100):
        """\
        Send AT command to the modem and parse the response
        if OK is found from response, that line is dropped
        """
        if type(at) is str:
            at = at.encode()
        logging.debug(f'at_cmd() -> {at}')
        self.serial.write(at)
        if not b'\r' in at:
            self.serial.write(b'\r')
        resp = []
        while True:
            line = self.serial.readline()
            if len(line) != 0:
                resp.append(line.decode())
                if b'OK\r\n' in line or b'ERROR' in line:
                    break
            else:
                timeout -= 1
                if timeout == 0:
                    break
        logging.debug(f'at_cmd() <- {"".join(resp).strip()}')
        if 'OK\r\n' not in resp[-1]:
            return None
        resp.pop()
        return ''.join(resp).strip()


class ModemShell:
    def __init__(self, serial):
        self.serial = serial
        if self.is_ready():
            self.cmd(b'ltelc nmodeauto -d')
            self.cmd(b'ltelc funmode -0')

    def is_ready(self):
        """\
        Check if the modem shell is ready, and seem to respond OK.
        """
        self.serial.readlines()
        (ok, _) = self.cmd('\r', timeout=1)
        if not ok:
            return False
        (ok, _) = self.cmd('shell colors off', timeout=1)
        if not ok:
            return False
        (ok, _) = self.cmd('shell echo off', timeout=1)
        return ok

    def cmd(self, cmd, timeout=100):
        """\
        Send a command to mosh command line.
        Return all the response lines in an array
        """
        (found, lines) = self.cmd_expect(cmd, 'uart-mosh:~\\$', timeout)
        if found:
            lines.pop()
        return (found, lines)

    def cmd_expect(self, cmd, expect, timeout=100, flush=False):
        """\
        Send a command to mosh command line and wait for expected line.
        expect is a regular expression.
        Return tuple (bool True if expected found, all the response lines in an array)
        """
        if type(cmd) is str:
            cmd = cmd.encode()
        resp = []
        found = False
        r = re.compile(expect)
        logging.debug(f'[mosh] -> {cmd.decode()}')
        self.serial.write(cmd)
        if not b'\r' in cmd:
            self.serial.write(b'\r')
        while True:
            line = self.serial.readline().decode()
            if len(line) != 0:
                #print("[mosh] <- %s" % line)
                resp.append(line)
                if r.search(line):
                    found = True
                    if not flush:
                        break
                    elif timeout > 1:
                        timeout = 1
            else:
                timeout -= 1
                if timeout == 0:
                    break
        logging.debug(f'[mosh] <- {"".join(resp)}')
        return (found, resp)

    def at_cmd(self, at):
        """\
        Send AT command to MoSH.
        Return the string response from AT command
        """
        # I need to double escape \n so the Zephyr shell does not eat it.
        at = at.rstrip().replace("\n", "\\x0A").replace(
            "\r", "\\x0D").replace("\"", "\\\"")
        cmd_str = 'at ' + '"' + at + '"'
        (ok, resp) = self.cmd(cmd_str)
        if not ok:
            return None
        if cmd_str in resp[0]:
            resp.pop(0)
        if 'OK\r\n' not in resp[-1]:
            return None
        resp.pop()
        return ''.join(resp).strip()


class MoSH:

    opcode = dict(
        write=0,
        list=1,
        read=2,
        delete=3
    )

    ktype = dict(
        rootca=0,
        clientCert=1,
        clientKey=2,
        pskKey=3,
        pskIdentity=4,
        pubKey=5,
        endorsementKey=8,
        ownershipKey=9,
        nordicIdentityRootCA=10
    )

    """\
    ModemShell communication helper.
    Uses PySerial to talk with the modem shell and send AT commands.
    """

    def __init__(self, serial_path, baudrate=115200):
        self.serial = serial.Serial(serial_path, baudrate, timeout=0.1)
        mosh = ModemShell(self.serial)
        at = ATclient(self.serial)
        if (at.is_ready()):
            logging.debug("AT client detected!")
            self.shell = at
        elif mosh.is_ready():
            logging.debug("Modem shell detected!")
            self.shell = mosh
        else:
            raise RuntimeError('Modem shell not responding')

    def is_ready(self):
        return self.shell.is_ready()

    def at_cmd(self, at):
        return self.shell.at_cmd(at)

    def cmd_expect(self, cmd, expect, timeout=100, flush=False):
        return self.shell.cmd_expect(cmd, expect, timeout, flush)
