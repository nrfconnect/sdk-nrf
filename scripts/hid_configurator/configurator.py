import os
import sys
import hid
import struct
import time

import argparse
import logging
import collections

from enum import IntEnum

REPORT_ID = 5
REPORT_SIZE = 30
EVENT_DATA_LEN_MAX = REPORT_SIZE - 6

NORDIC_VID = 0x1915
MOUSE_PID = 0x52DE
DONGLE_PID = 0x52DC
KEYBOARD_PID = 0x52DD

POLL_INTERVAL = 0.02
POLL_RETRY_COUNT = 200

TYPE_FIELD_POS = 0
GROUP_FIELD_POS = 6
EVENT_GROUP_SETUP = 0x1
EVENT_GROUP_DFU = 0x2

MOD_FIELD_POS = 3
SETUP_MODULE_SENSOR = 0x1
SETUP_MODULE_LED = 0x1

OPT_FIELD_POS = 0
SENSOR_OPT_CPI = 0x0
SENSOR_OPT_DOWNSHIFT_RUN = 0x1
SENSOR_OPT_DOWNSHIFT_REST1 = 0x2
SENSOR_OPT_DOWNSHIFT_REST2 = 0x3

DFU_DATA = 0x0
DFU_REBOOT = 0x1
DFU_IMGINFO = 0x2

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
        data_field_len = len(response_raw) - struct.calcsize('HBBB')

        if data_field_len < 0:
            logging.error('Response too short')
            return None

        # Report ID is not included in the feature report from device
        fmt = '<HBBB{}s'.format(data_field_len)

        (rcpt, event_id, status, data_len, data) = struct.unpack(fmt, response_raw)

        if data_len > len(data):
            logging.error('Required data not present')
            return None

        if data_len == 0:
            event_data = None
        else:
            event_data = data[:data_len]

        return Response(rcpt, event_id, status, event_data)


ConfigOption = collections.namedtuple('ConfigOption', 'range event_id help')
SENSOR_OPTIONS = {
    'downshift_run':    ConfigOption((10,   2550),   SENSOR_OPT_DOWNSHIFT_RUN,   'Time in milliseconds of switching from mode Run to Rest 1'),
    'downshift_rest1':  ConfigOption((320,  81600),  SENSOR_OPT_DOWNSHIFT_REST1, 'Time in milliseconds of switching from mode Rest 1 to Rest 2.'),
    'downshift_rest2':  ConfigOption((3200, 816000), SENSOR_OPT_DOWNSHIFT_REST2, 'Time in milliseconds of switching from mode Rest 2 to Rest 3.'),
    'cpi':              ConfigOption((100,  12000),  SENSOR_OPT_CPI,             'CPI resolution of a mouse sensor'),
}


def progress_bar(permil):
    LENGTH = 40
    done_len = LENGTH * permil // 1000
    progress_line = '[' + '*' * done_len + '-' * (LENGTH - done_len) + ']'
    percent = permil / 10.0
    print('\r{} {}%'.format(progress_line, percent), end='')


def check_range(value, value_range):
    if value > value_range[1] or value < value_range[0]:
        return False
    return True


def create_set_report(recipient, event_id, event_data):
    """ Function creating a report in order to set a specified configuration
        value.
        Recipient is a device product ID. """

    assert(type(recipient) == int)
    assert(type(event_id) == int)
    if event_data:
        assert(type(event_data) == bytes)
        event_data_len = len(event_data)

    status = ConfigStatus.PENDING
    report = struct.pack('<BHBBB', REPORT_ID, recipient, event_id, status,
                         event_data_len)
    if event_data:
        report += event_data

    assert(len(report) <= REPORT_SIZE)
    report += b'\0' * (REPORT_SIZE - len(report))

    return report


def create_fetch_report(recipient, event_id):
    """ Function for creating a report which requests fetching of
        a configuration value from a device.
        Recipient is a device product ID. """

    assert(type(recipient) == int)
    assert(type(event_id) == int)

    status = ConfigStatus.FETCH
    report = struct.pack('<BHBBB', REPORT_ID, recipient, event_id, status, 0)

    assert(len(report) <= REPORT_SIZE)
    report += b'\0' * (REPORT_SIZE - len(report))

    return report


def exchange_feature_report(dev, recipient, event_id, event_data, is_fetch):
    if is_fetch:
        data = create_fetch_report(recipient, event_id)
    else:
        data = create_set_report(recipient, event_id, event_data)

    dev.send_feature_report(data)

    for retry in range(POLL_RETRY_COUNT):
        time.sleep(POLL_INTERVAL)

        response_raw = dev.get_feature_report(REPORT_ID, REPORT_SIZE)
        response = Response.parse_response(response_raw)

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


def get_device_pid(device_type):
    if device_type == 'mouse':
        return MOUSE_PID
    elif device_type == 'keyboard':
        return KEYBOARD_PID
    elif device_type == 'dongle':
        return DONGLE_PID
    else:
        return None


def open_device(device_type):
    if device_type not in ['mouse', 'keyboard', 'dongle']:
        print("Unsupported device type")
        return None

    try:
        dev = hid.Device(vid=NORDIC_VID, pid=get_device_pid(device_type))
        print("Found device")
    except hid.HIDException:
        try:
            dev = hid.Device(vid=NORDIC_VID, pid=DONGLE_PID)
            print("Try to connect via dongle")
        except hid.HIDException:
            print("Cannot find selected device nor dongle")
            return None
    except:
        logging.error("Unknown error: {}".format(sys.exc_info()[0]))
        return None

    return dev


def perform_dfu(dev, args):
    dfu_image = args.dfu_image
    event_id = (EVENT_GROUP_DFU << GROUP_FIELD_POS) | (DFU_DATA << TYPE_FIELD_POS)
    recipient = get_device_pid(args.device_type)

    if not os.path.isfile(dfu_image):
        print('DFU image file does not exists')
        return

    img_length = os.stat(dfu_image).st_size
    print('DFU image size: {} bytes'.format(img_length))

    img_file = open(dfu_image, "rb")
    offset = 0

    try:
        while True:
            dfu_header = struct.pack('<II', img_length, offset)

            chunk_data = img_file.read(EVENT_DATA_LEN_MAX - len(dfu_header))
            chunk_len = len(chunk_data)
            if chunk_len == 0:
                break

            logging.debug('Send DFU request: offset {}, size {}'.format(offset, chunk_len))

            event_data = dfu_header + chunk_data

            progress_bar(int(offset/img_length * 1000))
            success = exchange_feature_report(dev, recipient, event_id, event_data, False)
            if not success:
                break

            offset += chunk_len
    except:
        success = False
    finally:
        img_file.close()

    if success:
        print('')
        print('DFU transfer completed')
    else:
        print('')
        print('DFU transfer failed')


def perform_config(dev, args):
    if args.module == 'sensor':
        module_id = SETUP_MODULE_SENSOR
        options = SENSOR_OPTIONS
    else:
        print('Unknown module')
        return

    config_name  = args.option
    config_opts  = options[config_name]

    value_range  = config_opts.range
    opt_id       = config_opts.event_id

    recipient = get_device_pid(args.device_type)
    event_id = (EVENT_GROUP_SETUP << GROUP_FIELD_POS) | (module_id << MOD_FIELD_POS) | (opt_id << OPT_FIELD_POS)

    if (args.value == 'fetch'):
        logging.debug('Fetch the current value of {} from the firmware'.format(config_name))
        try:
            success, fetched_data = exchange_feature_report(dev, recipient, event_id, None, True)
        except:
            success = False

        if success and fetched_data:
            val = int.from_bytes(fetched_data, byteorder='little')
            print('Fetched {}: {}'.format(config_name, val))
        else:
            print('Failed to fetch {}'.format(config_name))
    else:
        config_value = int(args.value)
        logging.debug('Send request to update {}: {}'.format(config_name, config_value))
        if not check_range(config_value, value_range):
            print('Failed. Config value for {} must be in range {}'.format(config_name, value_range))
            return

        event_data = struct.pack('<I', config_value)
        try:
            success = exchange_feature_report(dev, recipient, event_id, event_data, False)
        except:
            success = False

        if success:
            print('{} set to {}'.format(config_name, config_value))
        else:
            print('Failed to set {}'.format(config_name))


def perform_fwinfo(dev, args):
    event_id = (EVENT_GROUP_DFU << GROUP_FIELD_POS) | (DFU_IMGINFO << TYPE_FIELD_POS)
    recipient = get_device_pid(args.device_type)

    try:
        success, fetched_data = exchange_feature_report(dev, recipient, event_id, None, True)
    except:
        success = False

    if success and fetched_data:
        fmt = '<BIBBHI'
        assert(struct.calcsize(fmt) <= EVENT_DATA_LEN_MAX)

        (flash_area_id, image_len, ver_major, ver_minor, ver_rev, ver_build_nr) = struct.unpack(fmt, fetched_data)
        print(('Firmware info\n'
               '\tFLASH area id: {}\n'
               '\tImage length: {}\n'
               '\tVersion: {}.{}.{}.{}').format(flash_area_id,
                                                image_len,
                                                ver_major,
                                                ver_minor,
                                                ver_rev,
                                                ver_build_nr))
    else:
        print('FW info request failed')


def perform_fwreboot(dev, args):
    event_id = (EVENT_GROUP_DFU << GROUP_FIELD_POS) | (DFU_REBOOT << TYPE_FIELD_POS)
    recipient = get_device_pid(args.device_type)

    try:
        success, fetched_data = exchange_feature_report(dev, recipient, event_id, None, True)
    except:
        success = False

    if success:
        print('Firmware rebooted')
    else:
        print('FW reboot request failed')


def configurator():
    logging.basicConfig(level=logging.ERROR)
    logging.info('Configuration channel for nRF52 Desktop')

    parser = argparse.ArgumentParser()
    parser.add_argument('device_type', help='Selected device type: mouse/dongle')
    subparsers = parser.add_subparsers(dest='command')
    subparsers.required = True
    parser_dfu = subparsers.add_parser('dfu', help='Run DFU')
    parser_dfu.add_argument('dfu_image', type=str, help='Path to a DFU image')
    parser_fwinfo = subparsers.add_parser('fwinfo', help='Obtain information about FW image')
    parser_fwreboot = subparsers.add_parser('fwreboot', help='Request FW reboot')
    parser_config = subparsers.add_parser('config', help='Write configuration option')
    config_subparsers = parser_config.add_subparsers(dest='module')
    config_subparsers.required = True
    parser_config_sensor = config_subparsers.add_parser('sensor', help='Optical sensor options')
    parser_config_sensor_subparsers = parser_config_sensor.add_subparsers(dest='option')
    parser_config_sensor_subparsers.required = True
    for opt_name in SENSOR_OPTIONS.keys():
        parser_config_sensor_opt = parser_config_sensor_subparsers.add_parser(opt_name, help=SENSOR_OPTIONS[opt_name].help)
        parser_config_sensor_opt.add_argument('value', help='value range: {} or "fetch" to fetch the value from device'.format(SENSOR_OPTIONS[opt_name].range))
    args = parser.parse_args()

    dev = open_device(args.device_type)
    if not dev:
        return

    if args.command == 'dfu':
        perform_dfu(dev, args)
    elif args.command == 'fwinfo':
        perform_fwinfo(dev, args)
    elif args.command == 'fwreboot':
        perform_fwreboot(dev, args)
    elif args.command == 'config':
        perform_config(dev, args)

if __name__ == '__main__':
    try:
        configurator()
    except KeyboardInterrupt:
        pass
