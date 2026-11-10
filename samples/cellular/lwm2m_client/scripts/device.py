#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.9"
# dependencies = [
#     "pyserial>=3.5",
#     "python-dotenv",
# ]
# ///
#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Device requests"""

import argparse
import json
import logging
import os
import subprocess
import sys
import time

import serial

# Board (PCA) name to Zephyr build target
BOARDS = {
    "PCA10090": "nrf9160dk/nrf9160/ns",
    "PCA10153": "nrf9161dk/nrf9161/ns",
    "PCA10171": "nrf9151dk/nrf9151/ns",
    "PCA20065": "thingy91x/nrf9151/ns",
}

class Device:
    """Generate requests for device using AT commands"""
    def __init__(self):
        device = self.select_device()
        self.sid = device["serialNumber"].lstrip("0")
        self.pca = device["devkit"]["boardVersion"]
        self.com_port = self.get_com_port(device)
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
        cmd = f"west build --board {BOARDS[self.pca]} --build-dir build --pristine always {self.sample_path}"
        logging.info(f"{cmd}")
        subprocess.run(cmd.split(), cwd=self.sample_path, check=True, capture_output=True)
        logging.info("Done!")

    def flash_at_client(self):
        logging.info(f"Recovering and programming device {self.sid} at port {self.com_port}")
        cmd = "west flash --build-dir build --recover"
        subprocess.run(cmd.split(), cwd=self.sample_path, check=True, capture_output=True)
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
        devices = self.get_devices()

        # No connected devices, abort
        if len(devices) == 0:
            logging.error("No connected supported devices")
            sys.exit(1)

        # Only 1 connected device, return it
        if len(devices) == 1:
            return devices[0]

        # Multiple connected devices, make the user choose
        for i, x in enumerate(devices):
            print(f"{i + 1}) {x['serialNumber'].lstrip('0')} ({x['devkit']['boardVersion']})")

        while True:
            try:
                index = int(input("Select device (number): ")) - 1
            except ValueError:
                index = -1
            if index < 0 or index >= len(devices):
                print("Invalid choice, please try again")
                continue
            return devices[index]

    @classmethod
    def get_devices(cls):
        """List connected devices with a supported board version using nrfutil"""
        try:
            result = subprocess.run(["nrfutil", "device", "list", "--json"],
                                    check=True, capture_output=True, text=True)
        except FileNotFoundError:
            logging.error("nrfutil not found, install it and the 'device' command")
            sys.exit(1)
        except subprocess.CalledProcessError as e:
            logging.exception("nrfutil device list failed: %s", e.stderr.strip())
            sys.exit(1)

        devices = []
        for line in result.stdout.splitlines():
            try:
                msg = json.loads(line)
            except json.JSONDecodeError:
                continue
            if msg.get("type") != "info":
                continue
            for item in msg["data"].get("devices", []):
                pca = (item.get("devkit") or {}).get("boardVersion")
                if pca in BOARDS and item.get("serialPorts"):
                    devices.append(item)
        return devices

    @classmethod
    def get_com_port(cls, device):
        """Return the first virtual COM port of the device, used for AT commands"""
        ports = sorted(device["serialPorts"], key=lambda p: p["vcom"])
        return ports[0]["comName"]

if __name__ == "__main__":
    try:
        from dotenv import load_dotenv
        load_dotenv()
    except ImportError:
        pass

    def delete(args):
        """Delete content of sec_tag"""
        dev.delete_sec_tag(args.tag)

    def imei(_args):
        print(dev.get_imei())

    def read(args):
        for line in dev.read_sec_tag(args.tag):
            print(line)

    def list(_args):
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
