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
    DOWNSHIFT_RUN = 2
    DOWNSHIFT_REST1 = 3
    DOWNSHIFT_REST2 = 4


def check_range(value, lower, upper, value_name):
    if value > upper or value < lower:
        raise ValueError('{} {} out of range'.format(value_name, value))


def create_report(event_id, value):
    """ Function creating a HID feature report with 32-bit unsigned value """

    data = struct.pack('<BBI', REPORT_ID, event_id, value)
    return data


def configurator():
    logging.basicConfig(level=logging.DEBUG)
    logging.info('Configuration channel for nRF52 Desktop')

    parser = argparse.ArgumentParser()
    parser.add_argument('--cpi', type=int,
                        help='CPI resolution of a mouse sensor')
    parser.add_argument('--downshift_run', type=int,
                        help='time in milliseconds of switching from Run mode'
                             'to Rest 1 mode')
    parser.add_argument('--downshift_rest1', type=int,
                        help='time in milliseconds of switching from Rest 1 '
                             'mode to Rest 2 mode')
    parser.add_argument('--downshift_rest2', type=int,
                        help='time in milliseconds of switching from Rest 2 '
                             'mode to Rest 3 mode')
    args = parser.parse_args()

    # Open HID device
    dev = hid.Device(vid=MOUSE_VID, pid=MOUSE_PID)
    logging.debug('Opened device')

    if (args.cpi):
        name = 'CPI'
        value = args.cpi
        check_range(value, 100, 12000, name)

        event_id = ConfigEventID.MOUSE_CPI.value
        data = create_report(event_id, value)

        # Send configuration report
        dev.send_feature_report(data)
        logging.debug('Sent request to update {}: {}'.format(name, value))

    if (args.downshift_run):
        name = 'Downshift run time'
        value = args.downshift_run
        check_range(value, 10, 2550, name)

        event_id = ConfigEventID.DOWNSHIFT_RUN.value
        data = create_report(event_id, value)

        # Send configuration report
        dev.send_feature_report(data)
        logging.debug('Sent request to update {}: {}'.format(name, value))

    if (args.downshift_rest1):
        name = 'Downshift rest1 time'
        value = args.downshift_rest1
        check_range(value, 320, 81600, name)

        event_id = ConfigEventID.DOWNSHIFT_REST1.value
        data = create_report(event_id, value)

        # Send configuration report
        dev.send_feature_report(data)
        logging.debug('Sent request to update {}: {}'.format(name, value))

    if (args.downshift_rest2):
        name = 'Downshift rest2 time'
        value = args.downshift_rest2
        check_range(value, 3200, 816000, name)

        event_id = ConfigEventID.DOWNSHIFT_REST2.value
        data = create_report(event_id, value)

        # Send configuration report
        dev.send_feature_report(data)
        logging.debug('Sent request to update {}: {}'.format(name, value))


if __name__ == '__main__':
    configurator()
