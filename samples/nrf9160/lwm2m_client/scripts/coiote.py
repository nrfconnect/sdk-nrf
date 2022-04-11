#!/usr/bin/env python3
#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Module for interacting with AVSystem Coiote device management REST API"""

import os
import sys
import argparse
import logging
import json
import requests


class Coiote():
    """Interact with Coiote server"""
    def __init__(self):
        self.api_url = "https://eu.iot.avsystem.cloud/api/coiotedm/v3"
        self.user = os.environ.get("COIOTE_USER")
        self.passwd = os.environ.get("COIOTE_PASSWD")
        self.headers = {'accept': 'application/json',
                        'content-type': 'application/json'}
        self.domain = os.environ.get("COIOTE-DOMAIN")
        token = os.environ.get("COIOTE-TOKEN")

        if token is not None:
            self.headers = {'accept': 'application/json',
                            'content-type': 'application/json',
                            'authorization': f'Bearer {token}'}
        if self.user and self.passwd:
            self.auth = (self.user, self.passwd)
        else:
            self.auth = None

    def authenticate(self):
        """Request authentication token"""
        auth_url = "https://eu.iot.avsystem.cloud/api/auth/oauth_password"
        headers = {'content-type': 'application/x-www-form-urlencoded'}
        payload = f"grant_type=password&username={self.user}&password={self.passwd}"
        resp = requests.post(auth_url, headers=headers, data=payload)
        if resp.status_code != 201:
            logging.debug(resp.text)
            logging.error("Error in authentication")
            sys.exit(1)

        response = dict(json.loads(resp.text))
        token = response['access_token']
        token_expiry = response['expires_in']
        logging.info("Authenticated, token expires in %s s", str(token_expiry))

        self.headers = {'accept': 'application/json',
                        'content-type': 'application/json',
                        'authorization': f'Bearer {token}'}
        logging.info("Token: \n%s", token)

    def get(self, path):
        """Send HTTP GET query"""
        resp = requests.get(f"{self.api_url}{path}",
                            headers=self.headers, auth=self.auth)
        return Coiote.handle_response(resp)

    def put(self, path, data):
        """Send HTTP PUT query"""
        resp = requests.put(f"{self.api_url}{path}",
                            headers=self.headers, data=data, auth=self.auth)
        return Coiote.handle_response(resp)

    def post(self, path, data):
        """Send HTTP POST query"""
        resp = requests.post(f"{self.api_url}{path}",
                             headers=self.headers, data=data, auth=self.auth)
        return Coiote.handle_response(resp)

    def delete(self, path):
        """Send HTTP DELETE query"""
        resp = requests.delete(f"{self.api_url}{path}", headers={
                               'accept': 'application/json'}, auth=self.auth)
        return Coiote.handle_response(resp)

    @staticmethod
    def handle_response(resp):
        """Generic response handler for all queries"""
        if resp.text is None or len(resp.text) == 0:
            logging.warning('Response code {%d}', resp.status_code)
            return None
        obj = json.loads(resp.text)
        if resp.status_code >= 300 or resp.status_code < 200:
            logging.warning('Coiote: %s', obj['error'])
            return None
        logging.debug(json.dumps(obj, indent=4))
        return obj

    def get_device(self, dev_id):
        """Get device information"""
        return self.get('/devices/' + dev_id)

    def delete_device(self, dev_id):
        """Delete a device with given id"""
        if self.delete('/devices/' + dev_id) is None:
            logging.error('Coiote: Failed to delete device %s', dev_id)
        else:
            logging.info('Coiote: Deleted device %s', dev_id)

    def create_device(self, dev_id, psk):
        """Create a device to bootstrap server"""
        domain = self.get_domain()
        obj = {
            "id": dev_id + '-bs',
            "connectorType": "bootstrap",
            "properties": {
                "endpointName": dev_id
            },
            "domain": domain,
            "securityMode": "psk",
            "dtlsIdentity": dev_id,
            "dtlsPsk": {"HexadecimalPsk": psk}
        }
        if self.post('/devices', json.dumps(obj)) is None:
            logging.error('Coiote: Failed to create device %s', dev_id)
        else:
            logging.info(
                'Coiote: Creted device %s to domain %s', dev_id, domain)

    def get_domain(self):
        """Get list of domains for current user"""
        if self.domain is None:
            self.domain = self.get('/domains')[0]
            logging.debug('Coiote: Domain is %s', self.domain)
        return self.domain


if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG, format='%(message)s')

    try:
        from dotenv import load_dotenv
        load_dotenv()
    except ImportError:
        pass

    coiote = Coiote()

    def login(args):
        """Login to Coiote"""
        from getpass import getpass
        coiote.user = args.username
        coiote.passwd = getpass()
        coiote.authenticate()

    def get_domain(args):
        """Get domain"""
        domain = coiote.get_domain()
        if domain is not None:
            logging.info('Domain: %s', domain)

    def get(args):
        """Get a device"""
        logging.info(coiote.get_device(args.id))

    def delete(args):
        """Delete a device"""
        logging.info(coiote.delete_device(args.id))

    def create(args):
        """Create a device"""
        logging.info(coiote.create_device(args.id, args.psk))

    parser = argparse.ArgumentParser(
        description='Coiote device management')
    parser.set_defaults(func=None)
    subparsers = parser.add_subparsers(title='commands')
    login_pars = subparsers.add_parser(
        'login', help='Log in using username and password')
    login_pars.set_defaults(func=login)
    login_pars.add_argument('username', help='Coiote username')
    login_pars.add_argument(
        '-e', action='store_true', help='Read username and password environment variables')
    login_pars.add_argument(
        '-n', action='store_true', dest='no_store', help='Don\'t store the received token')

    get_pars = subparsers.add_parser('get', help='Get device data')
    get_pars.set_defaults(func=get)
    get_pars.add_argument('id', help='Device ID')
    del_pars = subparsers.add_parser('delete', help='Delete device')
    del_pars.set_defaults(func=delete)
    del_pars.add_argument('id', help='Device ID')
    create_pars = subparsers.add_parser('create', help='Create device')
    create_pars.set_defaults(func=create)
    create_pars.add_argument('id', help='Device ID')
    create_pars.add_argument('psk')
    create_pars.add_argument(
        '-b', action='store_true', help='Use bootstrap')
    get_domain_pars = subparsers.add_parser('domain', help='Get domain')
    get_domain_pars.set_defaults(func=get_domain)

    arg = parser.parse_args()
    if arg.func is None:
        parser.print_help()
        sys.exit(0)
    arg.func(arg)
