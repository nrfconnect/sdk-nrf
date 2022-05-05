#!/usr/bin/env python3
#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Device requests"""

import sys
import argparse
import logging
from atclient import ATclient

class Device:
    """Generate requests for device using AT commands"""
    def __init__(self, serial_path):
        self.at_client = ATclient(serial_path)

    def get_imei(self):
        """Request for IMEI code"""
        imei = self.at_client.at_cmd('AT+CGSN')
        if imei is None:
            logging.error("Failed to get IMEI")
            sys.exit(1)
        return imei

    def delete_sec_tag(self, tag):
        """Delete content of given sec_tag"""
        resp = self.at_client.at_cmd(f'AT%CMNG=1,{tag}')
        if len(resp) == 0:
            return
        for line in resp.split('\n'):
            logging.debug(line)
            content_type = line.split(',')[1]
            self.at_client.at_cmd(f'AT%CMNG=3,{tag},{content_type}')
            logging.debug(
                'Removed type %s from sec_tag %s', content_type, str(tag))
        logging.info('Security tag %s cleared', str(tag))

    def read_sec_tag(self, tag):
        """Read content of given sec_tag"""
        if tag is not None:
            resp = self.at_client.at_cmd(f'AT%CMNG=1,{tag}')
        else:
            resp = self.at_client.at_cmd(f'AT%CMNG=1')
        if len(resp) == 0:
            return []
        tags = []
        for line in resp.split('\n'):
            tags.append(line.split('%CMNG:')[1].strip())
        return tags

    def store_sec_tag(self, tag, content_type, content):
        """Write to sec_tag"""
        return self.at_client.at_cmd(f'AT%CMNG=0,{tag},{content_type},"{content}"')

    def store_psk(self, tag, identity, psk):
        """Write PSK key to given sec_tag"""
        self.store_sec_tag(tag, 4, identity)
        self.store_sec_tag(tag, 3, psk)
        logging.info('PSK credentials stored to sec_tag %d', int(tag))

if __name__ == "__main__":
    try:
        from dotenv import load_dotenv
        load_dotenv()
    except ImportError:
        pass

    def delete(args):
        """Delete content of sec_tag"""
        dev.delete_sec_tag(args.tag)

    def imei(args):
        print(dev.get_imei())

    def read(args):
        for line in dev.read_sec_tag(args.tag):
            print(line)

    def list(args):
        for line in dev.read_sec_tag(None):
            print(line)

    def store(args):
        dev.store_psk(args.tag, args.identity, args.psk)

    parser = argparse.ArgumentParser(
        description='nRF91 Secure Tag management')
    parser.set_defaults(func=None)
    parser.add_argument('serial', help='nRF91 Serial port')
    parser.add_argument('-d', help='Enable debug logs', action='store_true')

    subparsers = parser.add_subparsers(title='commands')

    imei_pars = subparsers.add_parser('imei', help='Read IMEI code')
    imei_pars.set_defaults(func=imei)

    del_pars = subparsers.add_parser(
        'del', help='Delete content of security tag')
    del_pars.set_defaults(func=delete)
    del_pars.add_argument('tag', help='Security tag')

    read_pars = subparsers.add_parser('read', help='Read content of security tag')
    read_pars.set_defaults(func=read)
    read_pars.add_argument('tag', help='Security tag')

    list_pars = subparsers.add_parser('list', help='Read content of all security tags')
    list_pars.set_defaults(func=list)

    store_pars = subparsers.add_parser('store', help='Store PSK to given sec_tag')
    store_pars.set_defaults(func=store)
    store_pars.add_argument('tag', help='Security tag')
    store_pars.add_argument('identity', help='Identity')
    store_pars.add_argument('psk', help='PSK key')

    arg = parser.parse_args()
    if arg.func is None:
        parser.print_help()
        sys.exit(0)

    if arg.d:
        logging.basicConfig(level=logging.DEBUG, format='%(message)s')
    else:
        logging.basicConfig(level=logging.INFO, format='%(message)s')

    dev = Device(arg.serial)
    arg.func(arg)
