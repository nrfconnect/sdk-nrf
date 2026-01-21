#!/usr/bin/env python3
#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Example provisioning script for nRF91 devices using
   AVSystem Coiote service
"""

import argparse
import contextlib
import logging
from secrets import token_hex

from atclient import ATclient
from coiote import Coiote
from device import Device
from leshan import Leshan

if __name__ == "__main__":
    try:
        from dotenv import load_dotenv
        load_dotenv()
    except ImportError:
        pass

    parser = argparse.ArgumentParser(
        description='nRF91 device provisioning example',
        allow_abbrev=False)
    parser.add_argument('-f', help='Build and flash the AT client', action='store_true')
    parser.add_argument('-d', help='Enable debug logs', action='store_true')
    parser.add_argument('-p', '--purge', dest='purge', help='Wipe the security tags and remove the device from the server', action='store_true')
    parser.add_argument('--leshan', help='Provision to Leshan demo server', action='store_true')
    parser.add_argument('--sec-tag', type=int, default=35724861, help='Security tag to use (default: 35724861)')
    parser.add_argument('--bs-sec-tag', type=int, default=35724862, help='Bootstrap security tag to use (default: 35724862)')
    args = parser.parse_args()

    if args.d:
        logging.basicConfig(level=logging.DEBUG, format='[%(levelname)s] %(filename)s - %(message)s')
    else:
        logging.basicConfig(level=logging.INFO, format='[%(levelname)s] %(filename)s - %(message)s')

    dev = Device()

    if args.f:
        dev.build_at_client()
        dev.flash_at_client()
        dev.wait_for_uart_string("Ready")

    dev.at_client = ATclient(dev.com_port)

    dev.at_client.at_cmd("AT+CFUN=4")

    imei = dev.get_imei()
    identity = f'urn:imei:{imei}'
    logging.info('Identity: %s', identity)

    # Remove previous keys
    dev.delete_sec_tag(args.sec_tag)
    dev.delete_sec_tag(args.bs_sec_tag)

    if args.leshan:
        leshan = Leshan('https://leshan.eclipseprojects.io/api')
        bserver = Leshan('https://leshan.eclipseprojects.io/bs/api')
        with contextlib.suppress(RuntimeError):
            leshan.delete_device(identity)
        with contextlib.suppress(RuntimeError):
            leshan.delete_bs_device(identity)
    else:
        coiote = Coiote()

        if coiote.get_device(identity):
            coiote.delete_device(identity)
        if coiote.get_device(identity + '-bs'):
            coiote.delete_device(identity + '-bs')

    if args.purge:
        exit()

    # Generate and store Bootstrap keys
    psk = token_hex(16)
    dev.store_psk(args.bs_sec_tag, identity, psk)

    if args.leshan:
        leshan.create_psk_device(identity, psk)
        bserver.create_bs_device(identity, 'coaps://leshan.eclipseprojects.io:5684', psk)
    else:
        # Provision the device
        coiote.create_device(identity, psk)
