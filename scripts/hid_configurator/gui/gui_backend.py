#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import sys
import time

import hid

# NrfHidDevice.py is in upper directory so we add it to path
sys.path.append('..')
from devices import DEVICE
from devices import get_device_pid, get_device_vid, get_device_type
from NrfHidDevice import NrfHidDevice
from modules.dfu import get_dfu_image_version, fwinfo, fwreboot, dfu_transfer
from modules.config import fetch_config, change_config


class Device:
    def __init__(self, device_type):
        self.type = device_type
        self.dev = NrfHidDevice(device_type,
                                get_device_vid(device_type),
                                get_device_pid(device_type),
                                get_device_pid('dongle'))

    @staticmethod
    def list_devices():
        devices = hid.enumerate()
        device_list = []
        for device in devices:
            device_type = get_device_type(device['product_id'])
            if device_type is not None and \
               get_device_vid(device_type) == device['vendor_id']:
                print("Add {} to device list".format(device_type))
                device_list.append(device_type)

        return device_list

    def perform_fwinfo(self):
        return fwinfo(self.dev)

    def perform_dfu_fwreboot(self, update_animation):
        fwreboot(self.dev)

        WAIT_TIME_MAX = 60
        CHECK_PERIOD = 0.5
        start_time = time.time()

        # Wait until device turns off
        while self.dev.initialized():
            self.dev.close_device()
            self.dev = NrfHidDevice(self.type,
                                    get_device_vid(self.type),
                                    get_device_pid(self.type),
                                    get_device_pid('dongle'))
            time.sleep(CHECK_PERIOD)

        while not self.dev.initialized():
            update_animation()
            self.dev = NrfHidDevice(self.type,
                                    get_device_vid(self.type),
                                    get_device_pid(self.type),
                                    get_device_pid('dongle'))
            if time.time() - start_time > WAIT_TIME_MAX:
                break
            time.sleep(CHECK_PERIOD)

        if self.dev.initialized():
            print('DFU completed')
        else:
            print('Cannot connect to device after reboot')

        return self.dev.initialized()

    def setcpi(self, value):
        opt_config = DEVICE[self.type]['config']['sensor']['options']['cpi']
        success = change_config(self.dev, 'sensor', 'cpi', value, opt_config)
        return success

    def fetchcpi(self):
        opt_config = DEVICE[self.type]['config']['sensor']['options']['cpi']
        success, val = fetch_config(self.dev, 'sensor', 'cpi', opt_config)
        if success:
            return val
        else:
            print("Fetch CPI failed")
            return False

    def perform_dfu(self, file, update_progressbar):
        dfu_image = file
        return dfu_transfer(self.dev, dfu_image, update_progressbar)

    @staticmethod
    def dfu_image_version(filepath):
        ver = get_dfu_image_version(filepath)
        if ver is None:
            return "Wrong image"
        else:
            v = '.'.join([str(i) for i in ver])
            return v


if __name__ == '__main__':
    print("Please run gui.py to start application")
