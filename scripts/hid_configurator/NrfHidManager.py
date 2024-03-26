#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from NrfHidDevice import NrfHidDevice

NORDIC_VID = 0x1915

class NrfHidManager():
    TYPE2BOARDLIST = {
        'gaming_mouse' : ['nrf52840gmouse', 'nrf52840dk', 'nrf54l15pdk'],
        'dongle' : ['nrf52840dongle', 'nrf52833dongle', 'nrf52820dongle', 'nrf5340dk'],
        'keyboard' : ['nrf52kbd'],
        'desktop_mouse' : ['nrf52dmouse', 'nrf52810dmouse'],
    }

    def __init__(self, vid=NORDIC_VID):
        self.devs = NrfHidDevice.open_devices(vid)

    @staticmethod
    def _get_dev_type(board_name):
        res = [k for k,v in NrfHidManager.TYPE2BOARDLIST.items()
               if board_name in v]

        if len(res) == 1:
            return res[0]
        else:
            if len(res) > 1:
                print('{} is assigned to more than one type'.format(board_name))
            return 'unknown'

    def list_devices(self):
        return ['Type: {} Board: {} (HW ID: {})'.format(NrfHidManager._get_dev_type(v.get_board_name()),
                                                        v.get_board_name(),
                                                        k)
                for k, v in self.devs.items()]

    def find_devices(self, device=None):
        if device is not None:
            try:
                board_names = NrfHidManager.TYPE2BOARDLIST[device]
            except Exception:
                board_names = [device]

            devs = [v for k,v in self.devs.items() if v.get_board_name() in board_names]

            if len(devs) == 0:
                try:
                    devs = [self.devs[device]]
                except Exception:
                    devs = []
        else:
            devs = list(self.devs.values())

        return devs

    def __del__(self):
        for d in self.devs:
            NrfHidDevice.close_device(self.devs[d])
