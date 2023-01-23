#!/usr/bin/env python3
#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import base64
import io
from enum import Enum
from typing import List

FUN_MODE_OFFLINE = 4

class CredType(Enum):
    ANY = -1
    ROOT_CA_CERT = 0
    CLIENT_CERT = 1
    CLIENT_KEY = 2
    PSK = 3
    PSK_ID = 4
    PUB_KEY = 5
    DEV_ID_PUB_KEY = 6
    RESERVED = 7
    ENDORSEMENT_KEY = 8
    OWNERSHIP_KEY = 9
    NORDIC_ID_ROOT_CA = 10
    NORDIC_PUB_KEY = 11

class Credential:
    def __init__(self, tag: int, type: int, sha: str):
        self.tag = tag
        self.type = CredType(type)
        self.sha = sha

def is_ok(response_lines):
    return len(response_lines) == 0

class CredStore:
    def __init__(self, at_client):
        self.at_client = at_client

    def func_mode(self, mode):
        """Set modem functioning mode. See AT Command Reference Guide for valid modes."""
        return self.at_client.at_command(f'AT+CFUN={mode}') == []

    def list(self, tag = None, type: CredType = CredType.ANY) -> List[Credential]:
        cmd = 'AT%CMNG=1'

        if tag is None and type != CredType.ANY:
            raise RuntimeError('Cannot list with type without a tag')

        # optional secure tag
        if tag is not None:
            cmd = f'{cmd},{tag}'

            # optional key type
            if type != CredType.ANY:
                cmd = f'{cmd},{CredType(type).value}'

        response_lines = self.at_client.at_command(cmd)
        columns = map(lambda line: line.replace('%CMNG: ', '').replace('"', '').split(','), response_lines)
        cred_map = map(lambda columns:
                Credential(int(columns[0]), int(columns[1]), columns[2].strip()),
                columns
            )

        return list(cred_map)

    def write(self, tag: int, type: CredType, file: io.TextIOBase):
        cert = '\n' + file.read().rstrip()
        return is_ok(self.at_client.at_command(f'AT%CMNG=0,{tag},{type.value},"{cert}"'))

    def delete(self, tag: int, type: CredType):
        return is_ok(self.at_client.at_command(f'AT%CMNG=3,{tag},{type.value}'))

    def keygen(self, tag: int, file: io.BufferedIOBase, attributes: str = ''):
        cmd = f'AT%KEYGEN={tag},2,0'

        if attributes != '':
            cmd = f'{cmd},"{attributes}"'

        response_lines = self.at_client.at_command(cmd)
        for l in response_lines:
            if not l.startswith('%KEYGEN'):
                continue
            keygen_output = l.replace('%KEYGEN: "', '')
            csr_der_b64 = keygen_output.split('.')[0]
            csr_der_bytes = base64.urlsafe_b64decode(csr_der_b64 + '===')
            file.write(csr_der_bytes)

        file.close()
