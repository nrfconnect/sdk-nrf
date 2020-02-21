#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import os
import sys
import hid
import struct
import time
import zlib
import random
import re

import logging
import collections
import imgtool.image as img

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

DFU_START = 0x0
DFU_DATA = 0x1
DFU_SYNC = 0x2
DFU_REBOOT = 0x3
DFU_IMGINFO = 0x4

LED_STREAM_DATA = 0x0

FLASH_PAGE_SIZE = 4096

POLL_INTERVAL_DEFAULT = 0.02
POLL_RETRY_COUNT = 200

DFU_SYNC_RETRIES = 3
DFU_SYNC_INTERVAL = 1

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

class FwInfo:
    def __init__(self, fetched_data):
        fmt = '<BIBBHI'
        assert struct.calcsize(fmt) <= EVENT_DATA_LEN_MAX
        vals = struct.unpack(fmt, fetched_data)
        flash_area_id, image_len, ver_major, ver_minor, ver_rev, ver_build_nr = vals
        self.flash_area_id = flash_area_id
        self.image_len = image_len
        self.ver_major = ver_major
        self.ver_minor = ver_minor
        self.ver_rev = ver_rev
        self.ver_build_nr = ver_build_nr

    def get_fw_version(self):
        return (self.ver_major, self.ver_minor, self.ver_rev, self.ver_build_nr)

    def __str__(self):
        return ('Firmware info\n'
                '  FLASH area id: {}\n'
                '  Image length: {}\n'
                '  Version: {}.{}.{}.{}').format(self.flash_area_id, self.image_len, self.ver_major, self.ver_minor,
                                                 self.ver_rev, self.ver_build_nr)

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

    assert type(recipient) == int
    assert type(event_id) == int
    if event_data:
        assert type(event_data) == bytes
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

    assert type(recipient) == int
    assert type(event_id) == int

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
    except:
        if not is_fetch:
            return False
        else:
            return False, None

    for retry in range(POLL_RETRY_COUNT):
        time.sleep(poll_interval)

        try:
            response_raw = dev.get_feature_report(REPORT_ID, REPORT_SIZE)
            response = Response.parse_response(response_raw)
        except:
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
        except:
            logging.error('Unknown error: {}'.format(sys.exc_info()[0]))

    if dev is None:
        print('Cannot find selected device nor dongle')
    elif i == device_type:
        print('Device found')
    else:
        print('Device connected via {}'.format(i))

    return dev


def fwinfo(dev, recipient):
    event_id = (EVENT_GROUP_DFU << GROUP_FIELD_POS) | (DFU_IMGINFO << TYPE_FIELD_POS)

    success, fetched_data = exchange_feature_report(dev, recipient, event_id, None, True)

    if success and fetched_data:
        fw_info = FwInfo(fetched_data)
        return fw_info
    else:
        return None


def fwreboot(dev, recipient):
    event_id = (EVENT_GROUP_DFU << GROUP_FIELD_POS) | (DFU_REBOOT << TYPE_FIELD_POS)
    success, _ = exchange_feature_report(dev, recipient, event_id, None, True)

    if success:
        logging.debug('Firmware rebooted')
    else:
        logging.debug('FW reboot request failed')

    return success


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

def dfu_sync(dev, recipient):
    event_id = (EVENT_GROUP_DFU << GROUP_FIELD_POS) | (DFU_SYNC << TYPE_FIELD_POS)

    success, fetched_data = exchange_feature_report(dev, recipient, event_id, None, True)

    if not success:
        return None

    fmt = '<BIII'
    assert struct.calcsize(fmt) <= EVENT_DATA_LEN_MAX

    if (fetched_data is None) or (len(fetched_data) < struct.calcsize(fmt)):
        return None

    return struct.unpack(fmt, fetched_data)


def dfu_start(dev, recipient, img_length, img_csum, offset):
    # Start DFU operation at selected offset.
    # It can happen that device will reject this request - this will be
    # verified by dfu sync at data exchange.
    event_id = (EVENT_GROUP_DFU << GROUP_FIELD_POS) | (DFU_START << TYPE_FIELD_POS)

    event_data = struct.pack('<III', img_length, img_csum, offset)

    success = exchange_feature_report(dev, recipient, event_id, event_data, False)

    if success:
        logging.debug('DFU started')
    else:
        logging.debug('DFU start failed')

    return success


def file_crc(dfu_image):
    crc32 = 1

    try:
        img_file = open(dfu_image, 'rb')
    except FileNotFoundError:
        print("Wrong file or file path")
        return None

    while True:
        chunk_data = img_file.read(512)
        if len(chunk_data) == 0:
            break
        crc32 = zlib.crc32(chunk_data, crc32)

    img_file.close()

    return crc32


def dfu_sync_wait(dev, recipient, is_active):
    if is_active:
        dfu_state = 0x01
    else:
        dfu_state = 0x00

    for i in range(DFU_SYNC_RETRIES):
        dfu_info = dfu_sync(dev, recipient)

        if dfu_info is not None:
            if dfu_info[0] != dfu_state:
                # DFU may be transiting its state. This can happen when previous
                # interrupted DFU operation is timing out. Sleep to allow it
                # to settle the state.
                time.sleep(DFU_SYNC_INTERVAL)
            else:
                break

    return dfu_info


def dfu_transfer(dev, recipient, dfu_image, progress_callback):
    img_length = os.stat(dfu_image).st_size
    dfu_info = dfu_sync_wait(dev, recipient, False)

    if not is_dfu_image_correct(dfu_image):
        return False

    if is_dfu_operation_pending(dfu_info):
        return False

    img_csum = file_crc(dfu_image)

    if not img_csum:
        return False

    offset = get_dfu_operation_offset(dfu_image, dfu_info, img_csum)

    success = dfu_start(dev, recipient, img_length, img_csum, offset)

    if not success:
        print('Cannot start DFU operation')
        return False

    img_file = open(dfu_image, 'rb')
    img_file.seek(offset)

    try:
        offset, success = send_chunk(dev, img_csum, img_file, img_length, offset, recipient, success, progress_callback)
    except:
        success = False

    img_file.close()
    print('')

    if success:
        print('DFU transfer completed')
        success = False

        dfu_info = dfu_sync_wait(dev, recipient, False)

        if dfu_info is None:
            print('Lost communication with the device')
        else:
            if dfu_info[0] != 0:
                print('Device holds DFU active')
            elif dfu_info[3] != offset:
                print('Device holds incorrect image offset')
            else:
                success = True
    return success


def send_chunk(dev, img_csum, img_file, img_length, offset, recipient, success, progress_callback):
    event_id = (EVENT_GROUP_DFU << GROUP_FIELD_POS) | (DFU_DATA << TYPE_FIELD_POS)

    while offset < img_length:
        if offset % FLASH_PAGE_SIZE == 0:
            # Sync DFU state at regular intervals to ensure everything
            # is all right.
            success = False
            dfu_info = dfu_sync(dev, recipient)

            if dfu_info is None:
                print('Lost communication with the device')
                break
            if dfu_info[0] == 0:
                print('DFU interrupted by device')
                break
            if (dfu_info[1] != img_length) or (dfu_info[2] != img_csum) or (dfu_info[3] != offset):
                print('Invalid sync information')
                break

        chunk_data = img_file.read(EVENT_DATA_LEN_MAX)
        chunk_len = len(chunk_data)

        if chunk_len == 0:
            break

        logging.debug('Send DFU request: offset {}, size {}'.format(offset, chunk_len))

        progress_callback(int(offset / img_length * 1000))

        success = exchange_feature_report(dev, recipient, event_id, chunk_data, False)

        if not success:
            print('Lost communication with the device')
            break

        offset += chunk_len

    return offset, success


def get_dfu_operation_offset(dfu_image, dfu_info, img_csum):
    # Check if the previously interrupted DFU operation can be resumed.
    img_length = os.stat(dfu_image).st_size

    if not img_csum:
        return None

    if (dfu_info[1] == img_length) and (dfu_info[2] == img_csum) and (dfu_info[3] <= img_length):
        print('Resume DFU at {}'.format(dfu_info[3]))
        offset = dfu_info[3]
    else:
        offset = 0

    return offset


def is_dfu_operation_pending(dfu_info):
    # Check there is no other DFU operation.
    if dfu_info is None:
        print('Cannot start DFU, device not responding')
        return True

    if dfu_info[0] != 0:
        print('Cannot start DFU. DFU in progress or memory is not clean.')
        print('Please stop ongoing DFU and wait until mouse cleans memory.')
        return True

    return False


def is_dfu_image_correct(dfu_image):
    if not os.path.isfile(dfu_image):
        print('DFU image file does not exists')
        return False

    img_length = os.stat(dfu_image).st_size

    if img_length <= 0:
        print('DFU image is empty')
        return False

    print('DFU image size: {} bytes'.format(img_length))

    res, _ = img.Image.verify(dfu_image, None)

    if res != img.VerifyResult.OK:
        print('DFU image is invalid')
        return False

    return True


def get_dfu_image_version(dfu_image):
    res, ver = img.Image.verify(dfu_image, None)

    if res != img.VerifyResult.OK:
        print('Image in file is invalid')
        return None

    return ver


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
