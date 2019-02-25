import hid
import struct
import time
from enum import Enum

import argparse
import logging

REPORT_ID = 4
REPORT_SIZE = 8

NORDIC_VID = 0x1915
MOUSE_PID = 0x52DE
DONGLE_PID = 0x52DC

POLL_INTERVAL = 0.5
POLL_MAX = 5


class ConfigEventID(Enum):
    MOUSE_CPI = 0
    DOWNSHIFT_RUN = 1
    DOWNSHIFT_REST1 = 2
    DOWNSHIFT_REST2 = 3

class ConfigForwardStatus(Enum):
	PENDING = 0
	SUCCESS = 1
	WRITE_ERROR = 2
	DISCONNECTED_ERROR = 3


def check_range(value, lower, upper, value_name):
    if value > upper or value < lower:
        raise ValueError('{} {} out of range'.format(value_name, value))


def create_report(recipient, event_id, value):
    """ Function creating a HID feature report with 32-bit unsigned value.
        Recipient is a device product ID. """

    data = struct.pack('<BHBI', REPORT_ID, recipient, event_id, value)
    assert(len(data) == REPORT_SIZE)
    return data


def write_feature_with_check(dev, data):
    feat_ready = ConfigForwardStatus(ConfigForwardStatus.PENDING)
    retries = 0

    while not feat_ready in (ConfigForwardStatus.SUCCESS, ConfigForwardStatus.DISCONNECTED_ERROR):
        dev.send_feature_report(data)

        time.sleep(POLL_INTERVAL)

        # Checking is useful when we write to USB dongle which forward configuration via BLE.
        response = dev.get_feature_report(REPORT_ID, REPORT_SIZE)
        feat_ready = ConfigForwardStatus(int.from_bytes(response, byteorder='little'))

        if feat_ready == ConfigForwardStatus.SUCCESS:
            logging.info('Success {}, retries {}'.format(feat_ready, retries))
        elif feat_ready == ConfigForwardStatus.DISCONNECTED_ERROR:
            logging.warning('Device disconnected, dongle cannot forward.')
            return
        else:
            # BLE write may fail if the requests from USB come too often. In such case, we retry.
            logging.info('Config event lost, retry...')

        retries += 1

        if (retries >= POLL_MAX):
            logging.error('Failed, too many retries')
            return


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

    # Open HID device. If mouse is not connected, try dongle
    try:
        dev = hid.Device(vid=NORDIC_VID, pid=MOUSE_PID)
        logging.info("Opened nRF52 Desktop Mouse")
    except hid.HIDException:
        dev = hid.Device(vid=NORDIC_VID, pid=DONGLE_PID)
        logging.info("Opened nRF52 Desktop Dongle")

    if (args.downshift_run):
        name = 'Downshift run time'
        value = args.downshift_run
        check_range(value, 10, 2550, name)

        event_id = ConfigEventID.DOWNSHIFT_RUN.value
        recipient = MOUSE_PID
        data = create_report(recipient, event_id, value)

        logging.debug('Send request to update {}: {}'.format(name, value))
        write_feature_with_check(dev, data)

    if (args.downshift_rest1):
        name = 'Downshift rest1 time'
        value = args.downshift_rest1
        check_range(value, 320, 81600, name)

        event_id = ConfigEventID.DOWNSHIFT_REST1.value
        recipient = MOUSE_PID
        data = create_report(recipient, event_id, value)

        logging.debug('Send request to update {}: {}'.format(name, value))
        write_feature_with_check(dev, data)

    if (args.downshift_rest2):
        name = 'Downshift rest2 time'
        value = args.downshift_rest2
        check_range(value, 3200, 816000, name)

        event_id = ConfigEventID.DOWNSHIFT_REST2.value
        recipient = MOUSE_PID
        data = create_report(recipient, event_id, value)

        logging.debug('Send request to update {}: {}'.format(name, value))
        write_feature_with_check(dev, data)

    if (args.cpi):
        name = 'CPI'
        value = args.cpi
        check_range(value, 100, 12000, name)

        event_id = ConfigEventID.MOUSE_CPI.value
        recipient = MOUSE_PID
        data = create_report(recipient, event_id, value)

        logging.debug('Send request to update {}: {}'.format(name, value))
        write_feature_with_check(dev, data)

if __name__ == '__main__':
    configurator()
