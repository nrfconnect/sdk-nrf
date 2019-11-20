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
from configurator_core import get_device_pid, get_device_vid, open_device, get_device_type, get_dfu_image_version
from configurator_core import fwinfo, fwreboot, fetch_config, change_config, dfu_transfer


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
            device_type = get_device_type(device['product_id'])
            if device_type is not None and \
               get_device_vid(device_type) == device['vendor_id']:
                    print("Add {} to device list".format(device_type))
                    device_list.append(device_type)

        return device_list

    def perform_fwinfo(self):
        return fwinfo(self.dev, self.pid)

    def perform_dfu_fwreboot(self, update_animation):
        fwreboot(self.dev, self.pid)
        self.dev.close()

        WAIT_TIME_MAX = 60
        CHECK_PERIOD = 0.5
        start_time = time.time()

        # Wait until device turns off
        while self.dev is not None:
            self.dev.close()
            self.dev = open_device(self.type)
            time.sleep(CHECK_PERIOD)

        while self.dev is None:
            update_animation()
            self.dev = open_device(self.type)
            if time.time() - start_time > WAIT_TIME_MAX:
                break
            time.sleep(CHECK_PERIOD)

        if self.dev is not None:
            print('DFU completed')
        else:
            print('Cannot connect to device after reboot')
        return (self.dev is not None)

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

    def dfu_image_version(self, filepath):
        ver = get_dfu_image_version(filepath)
        if ver is None:
            return "Wrong image"
        else:
            v = '.'.join([str(i) for i in ver])
            return v


if __name__ == '__main__':
    print("Please run gui.py to start application")
