#!/usr/bin/env python3
#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Device requests"""

import sys
import argparse
import logging
import os
import subprocess
import serial
import time
import re

from serial.tools.list_ports import comports

NRF91_VID = 4966

class Device:
    """Generate requests for device using AT commands"""
    def __init__(self):
        self.sid = self.select_device()
        self.com_port = self.get_com_ports(self.sid)[0]
        self.sample_path = os.path.join("/".join(os.path.abspath(__file__).split("/")[:-3]), "at_client")
        self.at_client = None

    def wait_for_uart_string(self, string, timeout=10):
        with serial.Serial(self.com_port, baudrate=115200, timeout=0.1) as uart:
            data = ""
            t_start, t_sleep = time.perf_counter(), 0.01
            while string not in data:
                data = uart.readline().decode()
                time.sleep(t_sleep)
                if time.perf_counter() - t_start > timeout:
                    logging.error(f"Timed out waiting for string {string}")
                    sys.exit(1)

    def build_at_client(self):
        logging.info("Building AT client")
        cmd = f"west build --board nrf9160dk_nrf9160_ns --build-dir build --pristine always {self.sample_path}"
        subprocess.run(cmd.split(), cwd=self.sample_path, check=True, capture_output=True)
        logging.info("Done!")

    def flash_at_client(self):
        logging.info(f"Erasing device {self.sid}")
        cmd = f"nrfjprog --snr {self.sid} --eraseall"
        subprocess.run(cmd.split(), check=True, capture_output=True)
        logging.info("Done!")

        logging.info(f"Programming device {self.sid} at port {self.com_port}")
        at_hex_path = os.path.join(self.sample_path, "build", "zephyr", "merged.hex")
        cmd = f"nrfjprog --snr {self.sid} --program {at_hex_path} --verify --reset"
        subprocess.run(cmd.split(), check=True, capture_output=True)
        logging.info("Done!")

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
            resp = self.at_client.at_cmd('AT%CMNG=1')
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

    def select_device(self):
        sids = self.get_devices(NRF91_VID)

        # No connected devices, abort
        if len(sids) == 0:
            logging.error("No connected devices")
            sys.exit(1)

        # Only 1 connected device, return it
        if len(sids) == 1:
            return sids[0]

        # Multiple connected devices, make the user choose
        for i, x in enumerate(sids):
            print("{}) {}".format(i + 1, x))

        while True:
            index = input("Select device (number): ")
            index = int(index) - 1
            if index < 0 or index >= len(sids):
                print("Invalid choice, please try again")
                continue
            break

        return sids[index]

    @classmethod
    def get_devices(cls, vid):
        sids = []
        for item in comports():
            if item.vid == vid:
                snr = item.serial_number.lstrip("0")
                if snr not in sids:
                    sids.append(snr)
        return sids

    @classmethod
    def get_com_ports(cls, sid):
        com_ports = []
        for item in comports():
            if item.manufacturer == "SEGGER" and item.serial_number.lstrip("0") == sid:
                com_ports.append(item.device)
        # Sort com ports on their integer identifiers, which takes care of corner case
        # where we have a cross-over from N digit to N + 1 digit identifiers
        return sorted(com_ports, key=lambda x: int(re.search(r"\d+", x).group(0)))

if __name__ == "__main__":
    try:
        from dotenv import load_dotenv
        load_dotenv()
    except ImportError:
        pass

    def delete(args):
        """Delete content of sec_tag"""
        dev.delete_sec_tag(args.tag)

    def imei():
        print(dev.get_imei())

    def read(args):
        for line in dev.read_sec_tag(args.tag):
            print(line)

    def list():
        for line in dev.read_sec_tag(None):
            print(line)

    def store(args):
        dev.store_psk(args.tag, args.identity, args.psk)

    parser = argparse.ArgumentParser(description='nRF91 Secure Tag management',
                                     allow_abbrev=False)
    parser.set_defaults(func=None)
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

    dev = Device()
    arg.func(arg)
