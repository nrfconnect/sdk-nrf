#!/usr/bin/env python3
#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Example provisioning script for nRF91 devices using
   AVSystem Coiote service
"""

import argparse
import logging
from secrets import token_hex
from coiote import Coiote
from device import Device


if __name__ == "__main__":
    try:
        from dotenv import load_dotenv
        load_dotenv()
    except ImportError:
        pass

    parser = argparse.ArgumentParser(
        description='nRF91 device provisioning example')
    parser.add_argument('serial', help='nRF91 Serial port')
    parser.add_argument('-d', help='Enable debug logs', action='store_true')
    parser.add_argument('-p', '--purge', dest='purge', help='Wipe the security tags and remove the device from the server', action='store_true')
    args = parser.parse_args()

    if args.d:
        logging.basicConfig(level=logging.DEBUG, format='%(message)s')
    else:
        logging.basicConfig(level=logging.INFO, format='%(message)s')

    dev = Device(args.serial)

    imei = dev.get_imei()
    identity = f'urn:imei:{imei}'
    logging.info('Identity: %s', identity)

    # Remove previous keys
    dev.delete_sec_tag(35724861)
    dev.delete_sec_tag(35724862)

    coiote = Coiote()
    coiote.get_device(identity)
    coiote.get_device(identity + '-bs')

    if coiote.get_device(identity):
        coiote.delete_device(identity)
    if coiote.get_device(identity + '-bs'):
        coiote.delete_device(identity + '-bs')

    if not args.purge:
        # Generate and store Bootstrap keys
        psk = token_hex(16)
        dev.store_psk(35724862, identity, psk)

        # Provision the device
        coiote.create_device(identity, psk)
