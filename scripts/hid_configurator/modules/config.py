#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import struct
import collections
import logging

from configurator_core import GROUP_FIELD_POS, EVENT_GROUP_SETUP, MOD_FIELD_POS, OPT_FIELD_POS
from configurator_core import EVENT_DATA_LEN_MAX
from configurator_core import exchange_feature_report
from devices import DEVICE, get_device_type


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
