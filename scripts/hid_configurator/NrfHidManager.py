#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

from NrfHidDevice import NrfHidDevice

NORDIC_VID = 0x1915

class NrfHidManager():
    TYPE2BOARDLIST = {
        'gaming_mouse' : ['nrf52840gmouse', 'nrf52840dk'],
        'dongle' : ['nrf52840dongle', 'nrf52833dongle', 'nrf52820dongle', 'nrf52840dk'],
        'keyboard' : ['nrf52kbd', 'nrf52840dk'],
        'desktop_mouse_nrf52832' : ['nrf52dmouse'],
        'desktop_mouse_nrf52810' : ['nrf52810dmouse']
    }

    def __init__(self, vid=NORDIC_VID):
        self.devs = NrfHidDevice.open_devices(vid)

    def list_devices(self):
        return ['Board: {} (HW ID: {})'.format(v.board_name, k)
                for k, v in self.devs.items()]

    def find_devices(self, device_type=None, hwid=None):
        if device_type is not None:
            try:
                board_names = NrfHidManager.TYPE2BOARDLIST[device_type]
            except Exception:
                board_names = [device_type]

            devs = { k:v for k,v in self.devs.items() if v.board_name in board_names}
        else:
            devs = self.devs

        if hwid is not None:
            try:
                devs = [devs[hwid]]
            except Exception:
                devs = []
        else:
            devs = list(devs.values())

        return devs

    def __del__(self):
        for d in self.devs:
            NrfHidDevice.close_device(self.devs[d])
