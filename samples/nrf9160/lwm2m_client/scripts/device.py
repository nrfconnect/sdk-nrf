#!/usr/bin/env python3
#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Device requests"""

import sys
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

    def store_sec_tag(self, tag, content_type, content):
        """Write to sec_tag"""
        return self.at_client.at_cmd(f'AT%CMNG=0,{tag},{content_type},"{content}"')

    def store_psk(self, tag, identity, psk):
        """Write PSK key to given sec_tag"""
        self.store_sec_tag(tag, 4, identity)
        self.store_sec_tag(tag, 3, psk)
        logging.info('PSK credentials stored to sec_tag %d', tag)
