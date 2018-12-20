import hid
import struct
from enum import Enum

import argparse
import logging

REPORT_ID = 4
MOUSE_VID = 0x1915
MOUSE_PID = 0x52DE


class ConfigEventID(Enum):
    MOUSE_CPI = 1


def configurator():
    logging.basicConfig(level=logging.DEBUG)
    logging.info('Configuration channel for nRF52 Desktop')

    parser = argparse.ArgumentParser()
    parser.add_argument('--cpi', type=int,
                        help='CPI resolution of a mouse sensor')
    args = parser.parse_args()

    # Open HID device
    dev = hid.Device(vid=MOUSE_VID, pid=MOUSE_PID)
    logging.debug('Opened device')

    if (args.cpi):
        data = create_report_mouse_cpi(args.cpi)

        # Send configuration report
        dev.send_feature_report(data)


def create_report_mouse_cpi(cpi):
    """ Function creating a HID feature report with 16-bit unsigned CPI resolution """
    if cpi > 12000 or cpi < 100:
        raise ValueError('CPI out of range')

    event_id = ConfigEventID.MOUSE_CPI.value

    data = struct.pack('<BBH', REPORT_ID, event_id, cpi)
    logging.debug('Sent request to update CPI: %d' % cpi)
    return data


if __name__ == '__main__':
    configurator()
