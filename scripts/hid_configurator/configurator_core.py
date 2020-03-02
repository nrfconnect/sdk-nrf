#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import sys
import hid
import struct
import time
from enum import IntEnum

import logging

from devices import get_device_pid, get_device_vid, get_device_type

REPORT_ID = 6
REPORT_SIZE = 30
EVENT_DATA_LEN_MAX = REPORT_SIZE - 6

POLL_INTERVAL_DEFAULT = 0.02
POLL_RETRY_COUNT = 200

TYPE_FIELD_POS = 0
GROUP_FIELD_POS = 6
EVENT_GROUP_SETUP = 0x1
EVENT_GROUP_DFU = 0x2
EVENT_GROUP_LED_STREAM = 0x3

MOD_FIELD_POS = 3
OPT_FIELD_POS = 0


class ConfigStatus(IntEnum):
    SUCCESS            = 0
    PENDING            = 1
    FETCH              = 2
    TIMEOUT            = 3
    REJECT             = 4
    WRITE_ERROR        = 5
    DISCONNECTED_ERROR = 6
    FAULT              = 99


class Response(object):
    def __init__(self, recipient, event_id, status, data):
        self.recipient = recipient
        self.event_id  = event_id
        self.status    = ConfigStatus(status)
        self.data      = data

    def __repr__(self):
        base_str = ('Response:\n'
                    '\trecipient 0x{:04x}\n'
                    '\tevent_id  0x{:02x}\n'
                    '\tstatus    {}\n').format(self.recipient,
                                               self.event_id,
                                               str(self.status))
        if self.data is None:
            data_str = '\tno data'
        else:
            data_str = ('\tdata_len {}\n'
                        '\tdata {}\n').format(len(self.data), self.data)

        return base_str + data_str

    @staticmethod
    def parse_response(response_raw):
        data_field_len = len(response_raw) - struct.calcsize('<BHBBB')

        if data_field_len < 0:
            logging.error('Response too short')
            return None

        # Report ID is not included in the feature report from device
        fmt = '<BHBBB{}s'.format(data_field_len)

        (report_id, rcpt, event_id, status, data_len, data) = struct.unpack(fmt, response_raw)

        if report_id != REPORT_ID:
            logging.error('Improper report ID')
            return None

        if data_len > len(data):
            logging.error('Required data not present')
            return None

        if data_len == 0:
            event_data = None
        else:
            event_data = data[:data_len]

        return Response(rcpt, event_id, status, event_data)


def create_set_report(recipient, event_id, event_data):
    """ Function creating a report in order to set a specified configuration
        value.
        Recipient is a device product ID. """

    assert isinstance(recipient, int)
    assert isinstance(event_id, int)
    if event_data:
        assert isinstance(event_data, bytes)
        event_data_len = len(event_data)
    else:
        event_data_len = 0

    status = ConfigStatus.PENDING
    report = struct.pack('<BHBBB', REPORT_ID, recipient, event_id, status,
                         event_data_len)
    if event_data:
        report += event_data

    assert len(report) <= REPORT_SIZE
    report += b'\0' * (REPORT_SIZE - len(report))

    return report


def create_fetch_report(recipient, event_id):
    """ Function for creating a report which requests fetching of
        a configuration value from a device.
        Recipient is a device product ID. """

    assert isinstance(recipient, int)
    assert isinstance(event_id, int)

    status = ConfigStatus.FETCH
    report = struct.pack('<BHBBB', REPORT_ID, recipient, event_id, status, 0)

    assert len(report) <= REPORT_SIZE
    report += b'\0' * (REPORT_SIZE - len(report))

    return report


def exchange_feature_report(dev, recipient, event_id, event_data, is_fetch,
                            poll_interval=POLL_INTERVAL_DEFAULT):
    if is_fetch:
        data = create_fetch_report(recipient, event_id)
    else:
        data = create_set_report(recipient, event_id, event_data)

    try:
        dev.send_feature_report(data)
    except Exception:
        if not is_fetch:
            return False
        else:
            return False, None

    for _ in range(POLL_RETRY_COUNT):
        time.sleep(poll_interval)

        try:
            response_raw = dev.get_feature_report(REPORT_ID, REPORT_SIZE)
            response = Response.parse_response(response_raw)
        except Exception:
            response = None

        if response is None:
            logging.error('Invalid response')
            op_status = ConfigStatus.FAULT
            break

        logging.debug('Parsed response: {}'.format(response))

        if (response.recipient != recipient) or (response.event_id != event_id):
            logging.error('Response does not match the request:\n'
                          '\trequest: recipient {} event_id {}\n'
                          '\tresponse: recipient {}, event_id {}'.format(recipient, event_id,
                                                                         response.recipient, response.event_id))
            op_status = ConfigStatus.FAULT
            break

        op_status = response.status
        if op_status != ConfigStatus.PENDING:
            break

    fetched_data = None
    success = False

    if op_status == ConfigStatus.SUCCESS:
        logging.info('Success')
        success = True
        if is_fetch:
            fetched_data = response.data
    else:
        logging.warning('Error: {}'.format(op_status.name))

    if is_fetch:
        return success, fetched_data

    return success


def open_device(device_type):
    dev = None

    devlist = [device_type]
    if 'dongle' not in devlist:
        devlist.append('dongle')

    for i in devlist:
        try:
            vid = get_device_vid(i)
            pid = get_device_pid(i)
            dev = hid.Device(vid=vid, pid=pid)
            break
        except hid.HIDException:
            pass
        except Exception:
            logging.error('Unknown error: {}'.format(sys.exc_info()[0]))

    if dev is None:
        print('Cannot find selected device nor dongle')
    elif i == device_type:
        print('Device found')
    else:
        print('Device connected via {}'.format(i))

    return dev


if __name__ == '__main__':
    print("Please run configurator_cli.py or gui.py to start application")
