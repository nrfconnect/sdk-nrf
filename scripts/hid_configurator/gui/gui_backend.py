#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import sys
import time

import hid

# configurator_core.py is in upper directory so we add it to path
sys.path.append('..')
from configurator_core import DEVICE
from configurator_core import get_device_pid, open_device, fwinfo, fwreboot, fetch_config, change_config, dfu_transfer,\
    get_device_type

GENERIC_DESKTOP_PAGE = 1
FWREBOOT_TIME = 25


class Device:
    def __init__(self, device_type):
        self.type = device_type
        self.pid = get_device_pid(device_type)
        self.dev = open_device(device_type)

    @staticmethod
    def list_devices():
        devices = hid.enumerate()
        device_list = []
        for device in devices:
            print(device)
            if device['usage_page'] == GENERIC_DESKTOP_PAGE:
                device_type = get_device_type(device['product_id'])
                if device_type:
                    print("Add {} to device list".format(device_type))
                    device_list.append(device_type)
        return device_list

    def perform_fwinfo(self):
        return fwinfo(self.dev, self.pid)

    def perform_dfu_fwreboot(self, update_progressbar):
        fwreboot(self.dev, self.pid)
        self.dev.close()
        t = 0
        while t < FWREBOOT_TIME:
            time.sleep(1)
            update_progressbar(t * 1 / FWREBOOT_TIME * 800)
            t += 1
        self.dev = None
        t = 1
        while self.dev is None:
            self.dev = open_device(self.type)
            update_progressbar(t * 1 / FWREBOOT_TIME * 800 + 800)
            time.sleep(1)
            t += 1/t
        update_progressbar(1000)
        print('DFU completed')
        return True


    def setcpi(self, value):
        config_name = 'cpi'
        module_config = DEVICE[self.type]['config']['sensor']
        options = module_config['options']
        module_id = module_config['id']
        recipient = self.pid
        dev = self.dev
        config_value = int(value)
        success = change_config(dev, recipient, config_name, config_value, options, module_id)
        return success

    def fetchcpi(self):
        config_name = 'cpi'
        module_config = DEVICE[self.type]['config']['sensor']
        options = module_config['options']
        module_id = module_config['id']
        recipient = self.pid
        dev = self.dev
        success, val = fetch_config(dev, recipient, config_name, options, module_id)
        if success:
            return val
        else:
            print("Fetch CPI failed")
            return False

    def perform_dfu(self, file, update_progressbar):
        dfu_image = file
        return dfu_transfer(self.dev, self.pid, dfu_image, update_progressbar)


if __name__ == '__main__':
    print("Please run gui.py to start application")
