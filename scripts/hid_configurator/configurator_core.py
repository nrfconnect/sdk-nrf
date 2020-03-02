#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import sys
import hid
import struct
import time
import random
import re

import logging
import collections

from enum import IntEnum

ConfigOption = collections.namedtuple('ConfigOption', 'range event_id help type')

REPORT_ID = 6
REPORT_SIZE = 30
EVENT_DATA_LEN_MAX = REPORT_SIZE - 6

TYPE_FIELD_POS = 0
GROUP_FIELD_POS = 6
EVENT_GROUP_SETUP = 0x1
EVENT_GROUP_DFU = 0x2
EVENT_GROUP_LED_STREAM = 0x3

MOD_FIELD_POS = 3
SETUP_MODULE_SENSOR = 0x1
SETUP_MODULE_QOS = 0x2
SETUP_MODULE_BLE_BOND = 0x3

OPT_FIELD_POS = 0
SENSOR_OPT_CPI = 0x0
SENSOR_OPT_DOWNSHIFT_RUN = 0x1
SENSOR_OPT_DOWNSHIFT_REST1 = 0x2
SENSOR_OPT_DOWNSHIFT_REST2 = 0x3

QOS_OPT_BLACKLIST = 0x0
QOS_OPT_CHMAP = 0x1
QOS_OPT_PARAM_BLE = 0x2
QOS_OPT_PARAM_WIFI = 0x3

BLE_BOND_PEER_ERASE = 0x0
BLE_BOND_PEER_SEARCH = 0x1

LED_STREAM_DATA = 0x0

POLL_INTERVAL_DEFAULT = 0.02
POLL_RETRY_COUNT = 200


PMW3360_OPTIONS = {
    'downshift_run':    ConfigOption((10,   2550),   SENSOR_OPT_DOWNSHIFT_RUN,   'Run to Rest 1 switch time [ms]', int),
    'downshift_rest1':  ConfigOption((320,  81600),  SENSOR_OPT_DOWNSHIFT_REST1, 'Rest 1 to Rest 2 switch time [ms]', int),
    'downshift_rest2':  ConfigOption((3200, 816000), SENSOR_OPT_DOWNSHIFT_REST2, 'Rest 2 to Rest 3 switch time [ms]', int),
    'cpi':              ConfigOption((100,  12000),  SENSOR_OPT_CPI,             'CPI resolution', int),
}

PAW3212_OPTIONS = {
    'sleep1_timeout':  ConfigOption((32,    512),    SENSOR_OPT_DOWNSHIFT_RUN,   'Sleep 1 switch time [ms]', int),
    'sleep2_timeout':  ConfigOption((20480, 327680), SENSOR_OPT_DOWNSHIFT_REST1, 'Sleep 2 switch time [ms]', int),
    'sleep3_timeout':  ConfigOption((20480, 327680), SENSOR_OPT_DOWNSHIFT_REST2, 'Sleep 3 switch time [ms]', int),
    'cpi':             ConfigOption((0,     2394),   SENSOR_OPT_CPI,             'CPI resolution', int),
}

QOS_OPTIONS = {
    'sample_count_min':       ConfigOption((0,      65535), QOS_OPT_PARAM_BLE,    'Minimum number of samples needed for channel map processing', int),
    'min_channel_count':      ConfigOption((2,      37),    QOS_OPT_PARAM_BLE,    'Minimum BLE channel count', int),
    'weight_crc_ok':          ConfigOption((-32767, 32767), QOS_OPT_PARAM_BLE,    'Weight of CRC OK [Fixed point with 1/100 scaling]', int),
    'weight_crc_error':       ConfigOption((-32767, 32767), QOS_OPT_PARAM_BLE,    'Weight of CRC ERROR [Fixed point with 1/100 scaling]', int),
    'ble_block_threshold':    ConfigOption((1,      65535), QOS_OPT_PARAM_BLE,    'Threshold relative to average rating for blocking BLE channels [Fixed point with 1/100 scaling]', int),
    'eval_max_count':         ConfigOption((1,      37),    QOS_OPT_PARAM_BLE,    'Maximum number of blocked channels that can be evaluated', int),
    'eval_duration':          ConfigOption((1,      65535), QOS_OPT_PARAM_BLE,    'Duration of channel evaluation [seconds]', int),
    'eval_keepout_duration':  ConfigOption((1,      65535), QOS_OPT_PARAM_BLE,    'Duration that a channel will be blocked before considered for re-evaluation [seconds]', int),
    'eval_success_threshold': ConfigOption((1,      65535), QOS_OPT_PARAM_BLE,    'Threshold relative to average rating for approving blocked BLE channel under evaluation [Fixed point with 1/100 scaling]', int),
    'wifi_rating_inc':        ConfigOption((1,      65535), QOS_OPT_PARAM_WIFI,   'Wifi strength rating multiplier. Increase value to block wifi faster [Fixed point with 1/100 scaling]', int),
    'wifi_present_threshold': ConfigOption((1,      65535), QOS_OPT_PARAM_WIFI,   'Threshold relative to average rating for considering a wifi present [Fixed point with 1/100 scaling]', int),
    'wifi_active_threshold':  ConfigOption((1,      65535), QOS_OPT_PARAM_WIFI,   'Threshold relative to average rating for considering a wifi active(blockable) [Fixed point with 1/100 scaling]', int),
    'channel_map':            ConfigOption(('0',      '0x1FFFFFFFFF'), QOS_OPT_CHMAP, '5-byte BLE channel map bitmask', str),
    'wifi_blacklist':         ConfigOption(('0',      '1,2,...,11'), QOS_OPT_BLACKLIST,'List of blacklisted wifi channels', str),
}

BLE_BOND_OPTIONS_DONGLE = {
    'peer_erase':             ConfigOption(None, BLE_BOND_PEER_ERASE,  'Trigger peer erase', None),
    'peer_search':            ConfigOption(None, BLE_BOND_PEER_SEARCH, 'Trigger peer search', None),
}

BLE_BOND_OPTIONS_DEVICE = {
    'peer_erase':             ConfigOption(None, BLE_BOND_PEER_ERASE,  'Trigger peer erase', None),
}

# Formatting details for QoS, which uses a struct containing multiple configuration values:
# OPTION_ID: (struct format, struct member names, binary to human-readable conversion function, human-readable to binary conversion function)
QOS_OPTIONS_FORMAT = {
    QOS_OPT_BLACKLIST: ('<H', ['wifi_blacklist'], lambda x: str([i for i in range(0,16) if x & (1 << i) != 0])[1:-1], lambda x: sum(map(lambda y: (1 << int(y)), re.findall(r'([\d]+)', x)))),
    QOS_OPT_CHMAP: ('<5s', ['channel_map'], lambda x: '0x{:02X}{:02X}{:02X}{:02X}{:02X}'.format(x[4],x[3],x[2],x[1],x[0]), None),
    QOS_OPT_PARAM_BLE: ('<HBhhHBHHH', ['sample_count_min', 'min_channel_count', 'weight_crc_ok', 'weight_crc_error', 'ble_block_threshold', 'eval_max_count', 'eval_duration', 'eval_keepout_duration', 'eval_success_threshold'], None, None),
    QOS_OPT_PARAM_WIFI: ('<HHH', ['wifi_rating_inc', 'wifi_present_threshold', 'wifi_active_threshold'], None, None),
}

PCA20041_CONFIG = {
    'sensor' : {
        'id' : SETUP_MODULE_SENSOR,
        'options' : PMW3360_OPTIONS
    },

    'ble_bond' : {
        'id' : SETUP_MODULE_BLE_BOND,
        'options' : BLE_BOND_OPTIONS_DEVICE
    }
}

PCA20044_CONFIG = {
    'sensor' : {
        'id' : SETUP_MODULE_SENSOR,
        'options' : PAW3212_OPTIONS
    },

    'ble_bond' : {
        'id' : SETUP_MODULE_BLE_BOND,
        'options' : BLE_BOND_OPTIONS_DEVICE
    }
}

PCA20045_CONFIG = {
    'sensor' : {
        'id' : SETUP_MODULE_SENSOR,
        'options' : PAW3212_OPTIONS
    },

    'ble_bond' : {
        'id' : SETUP_MODULE_BLE_BOND,
        'options' : BLE_BOND_OPTIONS_DEVICE
    }
}

PCA20037_CONFIG = {
    'ble_bond' : {
        'id' : SETUP_MODULE_BLE_BOND,
        'options' : BLE_BOND_OPTIONS_DEVICE
    }
}

PCA10059_CONFIG = {
    'qos' : {
        'id' : SETUP_MODULE_QOS,
        'options' : QOS_OPTIONS,
        'format' : QOS_OPTIONS_FORMAT
    },

    'ble_bond' : {
        'id' : SETUP_MODULE_BLE_BOND,
        'options' : BLE_BOND_OPTIONS_DONGLE
    }
}

DEVICE = {
    'desktop_mouse_nrf52832' : {
        'vid' : 0x1915,
        'pid' : 0x52DA,
        'config' : PCA20044_CONFIG,
        'stream_led_cnt' : 0,
    },
    'desktop_mouse_nrf52810' : {
        'vid' : 0x1915,
        'pid' : 0x52DB,
        'config' : PCA20045_CONFIG,
        'stream_led_cnt' : 0,
    },
    'gaming_mouse' : {
        'vid' : 0x1915,
        'pid' : 0x52DE,
        'config' : PCA20041_CONFIG,
        'stream_led_cnt' : 2,
    },
    'keyboard' : {
        'vid' : 0x1915,
        'pid' : 0x52DD,
        'config' : PCA20037_CONFIG,
        'stream_led_cnt' : 0,
    },
    'dongle' : {
        'vid' : 0x1915,
        'pid' : 0x52DC,
        'config' : PCA10059_CONFIG,
        'stream_led_cnt' : 0,
    }
}


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


class ConfigParser:
    """ Class used to simplify "read-modify-write" handling of parameter structs.
        For example when the python argument modifies one value within struct,
        and the remaining struct values should remain unchanged. """

    def __init__(self, fetched_data, fmt, member_names, value_presenter, value_formatter):
        assert struct.calcsize(fmt) <= EVENT_DATA_LEN_MAX
        self.fmt = fmt

        if value_presenter is not None:
            self.presenter = value_presenter
        else:
            self.presenter = lambda x: x

        if value_formatter is not None:
            self.formatter = value_formatter
        else:
            self.formatter = lambda x: x

        vals = struct.unpack(self.fmt, fetched_data)
        if len(member_names) != len(vals):
            raise ValueError('format does not match member name list')
        self.members = collections.OrderedDict()
        for key, val in zip(member_names, vals):
            self.members[key] = val

    def __str__(self):
        return ''.join(map(lambda x: '{}: {}\n'.format(x[0], x[1]), self.members.items()))

    def config_get(self, name):
        return self.presenter(self.members[name])

    def config_update(self, name, val):
        if name not in self.members:
            raise KeyError
        self.members[name] = self.formatter(val)

    def serialize(self):
        return struct.pack(self.fmt, *self.members.values())

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


def check_range(value, value_range):
    return value_range[0] <= value <= value_range[1]


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


def get_device_pid(device_type):
    return DEVICE[device_type]['pid']


def get_device_vid(device_type):
    return DEVICE[device_type]['vid']


def get_device_type(pid):
    for device_type in DEVICE:
        if DEVICE[device_type]['pid'] == pid:
            return device_type
    return None

def get_option_format(device_type, module_id):
    try:
        # Search through nested dicts and see if 'format' key exists for the config option
        format = [
            DEVICE[device_type]['config'][config]['format'] for config in DEVICE[device_type]['config'].keys()
                if DEVICE[device_type]['config'][config]['id'] == module_id
        ][0]
    except KeyError:
        format = None

    return format


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


def change_config(dev, recipient, config_name, config_value, device_options, module_id):
    config_opts = device_options[config_name]
    opt_id = config_opts.event_id
    event_id = (EVENT_GROUP_SETUP << GROUP_FIELD_POS) | (module_id << MOD_FIELD_POS) | (opt_id << OPT_FIELD_POS)
    value_range = config_opts.range
    logging.debug('Send request to update {}: {}'.format(config_name, config_value))

    dev_name = get_device_type(recipient)
    format = get_option_format(dev_name, module_id)

    if format is not None:
        # Read out first, then modify and write back (even if there is only one member in struct, to simplify code)
        success, fetched_data = exchange_feature_report(dev, recipient, event_id, None, True)
        if not success:
            return success

        try:
            config = ConfigParser(fetched_data, *format[opt_id])
            config.config_update(config_name, config_value)
            event_data = config.serialize()
        except (ValueError, KeyError):
            print('Failed. Invalid value for {}'.format(config_name))
            return False
    else:
        if config_value is not None:
            if not check_range(config_value, value_range):
                print('Failed. Config value for {} must be in range {}'.format(config_name, value_range))
                return False
            event_data = struct.pack('<I', config_value)
        else:
            event_data = None

    success = exchange_feature_report(dev, recipient, event_id, event_data, False)

    if success:
        logging.debug('Config changed')
    else:
        logging.debug('Config change failed')

    return success


def fetch_config(dev, recipient, config_name, device_options, module_id):
    config_opts = device_options[config_name]
    opt_id = config_opts.event_id
    event_id = (EVENT_GROUP_SETUP << GROUP_FIELD_POS) | (module_id << MOD_FIELD_POS) | (opt_id << OPT_FIELD_POS)
    logging.debug('Fetch the current value of {} from the firmware'.format(config_name))

    success, fetched_data = exchange_feature_report(dev, recipient, event_id, None, True)

    if not success or not fetched_data:
        return success, None

    dev_name = get_device_type(recipient)
    format = get_option_format(dev_name, module_id)

    if format is None:
        return success, config_opts.type.from_bytes(fetched_data, byteorder='little')
    else:
        return success, ConfigParser(fetched_data, *format[opt_id]).config_get(config_name)


class Step:
    def __init__(self, r, g, b, substep_count, substep_time):
        self.r = r
        self.g = g
        self.b = b
        self.substep_count = substep_count
        self.substep_time = substep_time

    def generate_random_color(self):
        self.r = random.randint(0, 255)
        self.g = random.randint(0, 255)
        self.b = random.randint(0, 255)


def led_send_single_step(dev, recipient, step, led_id):
    event_id = (EVENT_GROUP_LED_STREAM << GROUP_FIELD_POS) \
               | (LED_STREAM_DATA << TYPE_FIELD_POS)

    # Chosen data layout for struct is defined using format string.
    event_data = struct.pack('<BBBHHB', step.r, step.g, step.b,
                             step.substep_count, step.substep_time, led_id)

    success = exchange_feature_report(dev, recipient, event_id,
                                      event_data, False,
                                      poll_interval=0.001)

    return success


def fetch_free_steps_buffer_info(dev, recipient, led_id):
    event_id = (EVENT_GROUP_LED_STREAM << GROUP_FIELD_POS) \
               | (led_id << MOD_FIELD_POS)

    success, fetched_data = exchange_feature_report(dev, recipient,
                                                    event_id, None, True,
                                                    poll_interval=0.001)

    if (not success) or (fetched_data is None):
        return False, (None, None)

    # Chosen data layout for struct is defined using format string.
    fmt = '<B?'
    assert struct.calcsize(fmt) <= EVENT_DATA_LEN_MAX

    if len(fetched_data) != struct.calcsize(fmt):
        return False, (None, None)

    return success, struct.unpack(fmt, fetched_data)


if __name__ == '__main__':
    print("Please run configurator_cli.py or gui.py to start application")
