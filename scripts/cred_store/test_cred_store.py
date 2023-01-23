#!/usr/bin/env python3
#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import io
import pytest

from unittest.mock import Mock
from cred_store import *
from exceptions import ATCommandError

# pylint: disable=no-self-use
class TestCredStore:
    """Test suite for CredStore

    Most of the tests depend on a fixture that defines the at_client response instead of defining
    the response in each test.
    """

    @pytest.fixture
    def cred_store(self):
        self.at_client = Mock()
        return CredStore(self.at_client)

    @pytest.fixture
    def list_all_resp(self, cred_store):
        cred_store.at_client.at_command.return_value = [
            '%CMNG: 12345678, 0, "978C...02C4"',
            '%CMNG: 567890, 1, "C485...CF09"'
        ]

    @pytest.fixture
    def ok_resp(self, cred_store):
        cred_store.at_client.at_command.return_value = []

    @pytest.fixture
    def csr_resp(self, cred_store):
        # KEYGEN value is base64-encoded 'foo' and base64-encoded 'bar' joined by '.'
        cred_store.at_client.at_command.return_value = ['', '%KEYGEN: "Zm9v.YmFy"']

    @pytest.fixture
    def at_error(self, cred_store):
        cred_store.at_client.at_command.side_effect = ATCommandError('')

    def test_exposes_at_client(self, cred_store):
        assert cred_store.at_client is self.at_client

    def test_func_mode_offline(self, cred_store):
        cred_store.func_mode(4)
        self.at_client.at_command.assert_called_with('AT+CFUN=4')

    def test_func_mode_min(self, cred_store):
        cred_store.func_mode(0)
        self.at_client.at_command.assert_called_with('AT+CFUN=0')

    def test_func_mode_fail(self, cred_store, at_error):
        with pytest.raises(ATCommandError):
            cred_store.func_mode(4)

    def test_list_sends_cmng_command(self, cred_store, list_all_resp):
        cred_store.list()
        self.at_client.at_command.assert_called_with('AT%CMNG=1')

    def test_list_with_tag_part_of_cmng(self, cred_store, list_all_resp):
        cred_store.list(12345678)
        self.at_client.at_command.assert_called_with('AT%CMNG=1,12345678')

    def test_list_with_tag_and_type_part_of_cmng(self, cred_store, list_all_resp):
        cred_store.list(12345678, CredType(0))
        self.at_client.at_command.assert_called_with('AT%CMNG=1,12345678,0')

    def test_list_credentials_contains_tag(self, cred_store, list_all_resp):
        first = cred_store.list()[0]
        assert first.tag == 12345678

    def test_list_credentials_contains_type(self, cred_store, list_all_resp):
        first = cred_store.list()[0]
        assert first.type == CredType(0)

    def test_list_credentials_contains_sha(self, cred_store, list_all_resp):
        first = cred_store.list()[0]
        assert first.sha == '978C...02C4'

    def test_list_all_credentials_returns_multiple_credentials(self, cred_store, list_all_resp):
        result = cred_store.list()
        assert len(result) == 2
        assert result[0].sha == '978C...02C4'
        assert result[1].sha == 'C485...CF09'

    def test_list_type_without_tag(self, cred_store):
        with pytest.raises(RuntimeError):
            cred_store.list(None, CredType(0))

    def test_list_fail(self, cred_store, at_error):
        with pytest.raises(ATCommandError):
            cred_store.list()

    def test_delete_success(self, cred_store, ok_resp):
        assert cred_store.delete(567890, CredType(1))
        self.at_client.at_command.assert_called_with('AT%CMNG=3,567890,1')

    def test_delete_fail(self, cred_store, at_error):
        with pytest.raises(ATCommandError):
            cred_store.delete(123, CredType(1))

    def test_write_success(self, cred_store, ok_resp):
        cert_text = '''-----BEGIN CERTIFICATE-----
dGVzdA==
-----END CERTIFICATE-----'''
        fake_file = io.StringIO(cert_text)
        assert cred_store.write(567890, CredType.CLIENT_KEY, fake_file)
        self.at_client.at_command.assert_called_with(f'AT%CMNG=0,567890,2,"\n{cert_text}"')

    def test_write_fail(self, cred_store, at_error):
        with pytest.raises(ATCommandError):
            cred_store.write(567890, CredType.CLIENT_KEY, io.StringIO())

    def test_generate_sends_keygen_cmd(self, cred_store, csr_resp):
        fake_binary_file = Mock()
        cred_store.keygen(12345678, fake_binary_file)
        self.at_client.at_command.assert_called_with(f'AT%KEYGEN=12345678,2,0')

    def test_generate_with_attributes(self, cred_store, csr_resp):
        cred_store.keygen(12345678, Mock(), 'O=Nordic Semiconductor,L=Trondheim,C=no')
        self.at_client.at_command.assert_called_with(
            f'AT%KEYGEN=12345678,2,0,"O=Nordic Semiconductor,L=Trondheim,C=no"')

    def test_generate_writes_csr_to_stream(self, cred_store, csr_resp):
        fake_binary_file = Mock()
        cred_store.keygen(12345678, fake_binary_file)
        fake_binary_file.write.assert_called_with(b'foo')

    def test_generate_fail(self, cred_store, at_error):
        with pytest.raises(ATCommandError):
            cred_store.keygen(12345678, Mock())
