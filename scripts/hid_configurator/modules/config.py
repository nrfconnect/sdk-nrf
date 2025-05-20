#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import struct
import collections
import logging

from NrfHidDevice import EVENT_DATA_LEN_MAX
from modules.module_config import MODULE_CONFIG


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


def check_range(value, value_range):
    return value_range[0] <= value <= value_range[1]


def get_option_format(module_name, option_name_on_device):
    try:
        # Search through nested dicts and see if 'format' key exists for the config option
        format = MODULE_CONFIG[module_name]['format'][option_name_on_device]
    except KeyError:
        format = None

    return format

def change_config(dev, module_name, option_name, value, option_descr):
    value_range = option_descr.range
    logging.debug('Send request to update {}/{}: {}'.format(module_name,
                                                            option_name,
                                                            value))

    option_name_on_device = option_descr.option_name
    format = get_option_format(module_name, option_name_on_device)

    if format is not None:
        # Read out first, then modify and write back (even if there is only one member in struct, to simplify code)
        success, fetched_data = dev.config_get(module_name, option_name_on_device)
        if not success:
            return success

        try:
            config = ConfigParser(fetched_data, *format)
            config.config_update(option_name, value)
            event_data = config.serialize()
        except (ValueError, KeyError):
            print('Failed. Invalid value for {}/{}'.format(module_name, option_name))
            return False
    else:
        if value is not None:
            if not check_range(value, value_range):
                print('Failed. Config value for {}/{} must be in range {}'.format(module_name,
                                                                                  option_name,
                                                                                  value_range))
                return False

            event_data = struct.pack('<I', value)
        else:
            event_data = None

    success = dev.config_set(module_name, option_name_on_device, event_data)

    if success:
        logging.debug('Config changed')
    else:
        logging.debug('Config change failed')

    return success


def fetch_config(dev, module_name, option_name, option_descr):
    logging.debug('Fetch the current value of {}/{} from the firmware'.format(module_name,
                                                                              option_name))

    option_name_on_device = option_descr.option_name
    success, fetched_data = dev.config_get(module_name, option_name_on_device)

    if not success or not fetched_data:
        return success, None

    format = get_option_format(module_name, option_name_on_device)

    if format is None:
        return success, option_descr.type.from_bytes(fetched_data, byteorder='little')
    else:
        return success, ConfigParser(fetched_data, *format).config_get(option_name)
