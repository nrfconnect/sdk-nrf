#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import re
import collections

SETUP_MODULE_SENSOR = 0x1
SETUP_MODULE_QOS = 0x2
SETUP_MODULE_BLE_BOND = 0x3

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


ConfigOption = collections.namedtuple('ConfigOption', 'range event_id help type')

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


def get_device_pid(device_type):
    return DEVICE[device_type]['pid']


def get_device_vid(device_type):
    return DEVICE[device_type]['vid']


def get_device_type(pid):
    for device_type in DEVICE:
        if DEVICE[device_type]['pid'] == pid:
            return device_type
    return None
