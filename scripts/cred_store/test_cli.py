#!/usr/bin/env python3
#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import pytest

from unittest.mock import Mock, ANY, patch
from cli import main

from cred_store import CredType
from exceptions import NoATClientException

# pylint: disable=no-self-use
class TestCli():

    @pytest.fixture
    def at_client(self):
        at_client = Mock()
        return at_client

    @pytest.fixture
    def cred_store(self, at_client):
        cred_store = Mock()
        cred_store.at_client = at_client
        return cred_store

    @pytest.fixture
    def offline(self, cred_store):
        cred_store.funk_mode.return_value = True

    @pytest.fixture
    def empty_cred_list(self, cred_store):
        cred_store.list.return_value = []

    def test_device_passed_to_connect(self, cred_store, at_client, offline, empty_cred_list):
        main(['/dev/tty.usb', 'list'], cred_store)
        at_client.connect.assert_called_with('/dev/tty.usb', ANY, ANY)

    def test_baudrate_passed_to_connect(self, cred_store, at_client, offline, empty_cred_list):
        main(['fakedev', '--baudrate', '9600', 'list'], cred_store)
        at_client.connect.assert_called_with(ANY, 9600, ANY)

    def test_timeout_passed_to_connect(self, cred_store, at_client, offline, empty_cred_list):
        main(['fakedev', '--timeout', '3', 'list'], cred_store)
        at_client.connect.assert_called_with(ANY, ANY, 3)

    def test_at_client_verify(self, cred_store, at_client, offline, empty_cred_list):
        main(['fakedev', 'list'], cred_store)
        at_client.verify.assert_called()

    @patch('builtins.print')
    def test_at_client_verify_fail(self, mock_print, cred_store, at_client):
        at_client.verify.side_effect = NoATClientException()
        main(['fakedev', 'list'], cred_store)
        assert 'does not respond to AT commands' in mock_print.call_args[0][0]

    def test_at_client_enable_error_codes(self, cred_store, at_client, offline, empty_cred_list):
        main(['fakedev', 'list'], cred_store)
        at_client.enable_error_codes.assert_called()

    def test_list_default(self, cred_store, offline, empty_cred_list):
        main(['fakedev', 'list'], cred_store)
        cred_store.list.assert_called_with(None, CredType.ANY)

    def test_list_with_tag(self, cred_store, offline, empty_cred_list):
        main(['fakedev', 'list', '--tag', '123'], cred_store)
        cred_store.list.assert_called_with(123, ANY)

    def test_list_with_type(self, cred_store, offline, empty_cred_list):
        main(['fakedev', 'list', '--tag', '123', '--type', 'CLIENT_KEY'], cred_store)
        cred_store.list.assert_called_with(ANY, CredType.CLIENT_KEY)

    def test_write_tag_and_type(self, cred_store, offline):
        cred_store.write.return_value = True
        main(['fakedev', 'write', '123', 'ROOT_CA_CERT', 'test_cli.py'], cred_store)
        cred_store.write.assert_called_with(123, CredType.ROOT_CA_CERT, ANY)

    @patch('builtins.open')
    def test_write_file(self, mock_file, cred_store, offline):
        cred_store.write.return_value = True
        main(['fakedev', 'write', '123', 'ROOT_CA_CERT', 'foo.pem'], cred_store)
        mock_file.assert_called_with('foo.pem', 'r', ANY, ANY, ANY)

    def test_delete(self, cred_store, offline):
        cred_store.delete.return_value = True
        main(['fakedev', 'delete', '123', 'CLIENT_KEY'], cred_store)
        cred_store.delete.assert_called_with(123, CredType.CLIENT_KEY)

    @patch('builtins.open')
    def test_generate_tag(self, mock_file, cred_store, offline):
        cred_store.keygen.return_value = True
        main(['fakedev', 'generate', '123', 'foo.der'], cred_store)
        cred_store.keygen.assert_called_with(123, ANY)

    @patch('builtins.open')
    def test_generate_file(self, mock_file, cred_store, offline):
        cred_store.keygen.return_value = True
        main(['fakedev', 'generate', '123', 'foo.der'], cred_store)
        mock_file.assert_called_with('foo.der', 'wb', ANY, ANY, ANY)
