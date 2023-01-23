#!/usr/bin/env python3
#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""
Test for cert_manager.py
"""

import pytest

from unittest.mock import Mock
from at_client import ATClient
from exceptions import ATCommandError, NoATClientException

def encode_cmd(cmd):
    return f'{cmd}\r\n'.encode('utf-8')

def response_lines(lines):
    return map(encode_cmd, lines)

# pylint: disable=no-self-use
class TestATClient:

    @pytest.fixture
    def at_client(self):
        self.serial = Mock()
        return ATClient(self.serial)

    @pytest.fixture
    def ok_resp(self, at_client):
        self.serial.readline.return_value = 'OK\r\n'.encode('utf-8')

    @pytest.fixture
    def not_at_client_resp(self, at_client):
        self.serial.readline.side_effect = [b'AT: command not found', b'']

    @pytest.fixture
    def error_resp(self, at_client):
        self.serial.readline.return_value = 'ERROR\r\n'.encode('utf-8')

    @pytest.fixture
    def error_code_resp(self, at_client):
        self.serial.readline.return_value = '+CME ERROR: 514\r\n'.encode('utf-8')

    @pytest.fixture
    def error_ok_resp(self, at_client):
        self.serial.readline.side_effect = [b'ERROR', b'OK']

    def test_create_without_serial_device(self):
        with pytest.raises(RuntimeError):
            ATClient(None)

    def test_at_command_with_serial_closed(self, at_client):
        with pytest.raises(ConnectionError):
            self.serial.is_open = False
            at_client.at_command('AT')

    def test_verify_with_serial_closed(self, at_client):
        with pytest.raises(ConnectionError):
            self.serial.is_open = False
            at_client.verify()

    def test_exposes_serial_device(self, at_client):
        assert at_client.device is self.serial

    def test_connect_sets_port(self, at_client):
        at_client.connect('/dev/tty.usb')
        assert self.serial.port == '/dev/tty.usb'

    def test_connect_sets_baudrate(self, at_client):
        at_client.connect('foo', 123)
        assert self.serial.baudrate == 123

    def test_connect_sets_timeout(self, at_client):
        at_client.connect('foo', timeout=3)
        assert self.serial.timeout == 3

    def test_connect_opens_serial_connection_to_device(self, at_client):
        at_client.connect('foo')
        self.serial.open.assert_called()

    def test_verify_fails_for_wrong_response(self, at_client, not_at_client_resp):
        """Test that the AT client verifiation raises NoATClientException when readline returns
        unexpected lines and an empty string (timeout).
        """
        with pytest.raises(NoATClientException):
            at_client.verify()

    def test_verify_sends_at_command(self, at_client, ok_resp):
        at_client.verify()
        self.serial.write.assert_called_with('AT\r\n'.encode('utf-8'))

    def test_verify_fails_on_error(self, at_client, error_resp):
        with pytest.raises(NoATClientException):
            at_client.verify()

    def test_verify_retries_on_first_error(self, at_client, error_ok_resp):
        assert at_client.verify()

    def test_enable_error_codes_sends_cmd(self, at_client, ok_resp):
        at_client.enable_error_codes()
        self.serial.write.assert_called_with(encode_cmd('AT+CMEE=1'))

    def test_at_command_error(self, at_client, error_resp):
        with pytest.raises(ATCommandError):
            at_client.at_command('AT')

    def test_at_command_error_code(self, at_client, error_code_resp):
        with pytest.raises(ATCommandError):
            at_client.at_command('AT')

    def test_at_command_error_code_maps_to_message(self, at_client, error_code_resp):
        with pytest.raises(ATCommandError) as excinfo:
            at_client.at_command('AT')
        assert 'Not allowed' in str(excinfo.value)

    def test_at_command_with_single_line_response(self, at_client):
        self.serial.readline.side_effect = [b'single', b'OK']
        assert at_client.at_command('AT+CGSN') == ['single']

    def test_at_command_with_multi_line_response(self, at_client):
        self.serial.readline.side_effect = [b'foo', b'bar', b'OK']
        assert at_client.at_command('AT+CGSN') == ['foo', 'bar']
