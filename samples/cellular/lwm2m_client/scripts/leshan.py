# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import json
import requests

class Leshan:
    def __init__(self, url):
        self.api_url = url
        self.timeout = 60
        self.format = 'TLV'
        # self.format = "SENML_CBOR"

    @staticmethod
    def handle_response(resp):
        """Generic response handler for all queries"""
        if resp.status_code >= 300 or resp.status_code < 200:
            raise RuntimeError(f'Error {resp.status_code}: {resp.text}')
        if len(resp.text):
            obj = json.loads(resp.text)
            return obj
        else:
            return None

    def get(self, path):
        """Send HTTP GET query"""
        resp = requests.get(f'{self.api_url}{path}?timeout={self.timeout}&format={self.format}')
        return Leshan.handle_response(resp)

    def put(self, path, data):
        resp = requests.put(f'{self.api_url}{path}?timeout={self.timeout}&format={self.format}', data=data, headers={'content-type': 'application/json'})
        return Leshan.handle_response(resp)

    def post(self, path, data = None):
        resp = requests.post(f'{self.api_url}{path}', data=data, headers={'content-type': 'application/json'})
        return Leshan.handle_response(resp)

    def delete(self, path):
        resp = requests.delete(f'{self.api_url}{path}')
        return Leshan.handle_response(resp)

    def execute(self, endpoint, path):
        return self.post(f'/clients/{endpoint}/{path}')

    def write(self, endpoint, path, value):
        if isinstance(value, int):
            type = 'integer'
            value = str(value)
        elif isinstance(value, bool):
            type = 'boolean'
            value = "true" if value else "false"
        elif isinstance(value, str):
            type = 'string'
        id = path.split('/')[2]
        self.put(f'/clients/{endpoint}/{path}', f'{{"id":{id},"kind":"singleResource","value":"{value}","type":"{type}"}}')

    def create_psk_device(self, endpoint, psk):
        self.put('/security/clients/', f'{{"endpoint":"{endpoint}","tls":{{"mode":"psk","details":{{"identity":"{endpoint}","key":"{psk}"}} }} }}')

    def delete_device(self, endpoint):
        self.delete(f'/security/clients/{endpoint}')

    def create_bs_device(self, endpoint, server_uri, psk):
        data = f'{{"tls":{{"mode":"psk","details":{{"identity":"{endpoint}","key":"{psk}"}}}},"endpoint":"{endpoint}"}}'
        self.put('/security/clients/', data)
        id = str([ord(n) for n in endpoint])
        key = str([n for n in bytes.fromhex(psk)])
        content = '{"servers":{"0":{"binding":"U","defaultMinPeriod":1,"lifetime":60,"notifIfDisabled":true,"shortId":123}},"security":{"1":{"bootstrapServer":false,"clientOldOffTime":1,"publicKeyOrId":' + id + ',"secretKey":' + key + ',"securityMode":"PSK","serverId":123,"serverSmsNumber":"","smsBindingKeyParam":[],"smsBindingKeySecret":[],"smsSecurityMode":"NO_SEC","uri":"'+server_uri+'"}},"oscore":{},"toDelete":["/0","/1"]}'
        self.post(f'/bootstrap/{endpoint}', content)

    def delete_bs_device(self, endpoint):
        self.delete(f'/security/clients/{endpoint}')
        self.delete(f'/bootstrap/{endpoint}')
