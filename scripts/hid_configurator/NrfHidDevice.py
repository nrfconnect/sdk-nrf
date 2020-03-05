#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import hid
import struct
import time
import logging
from enum import IntEnum

REPORT_ID = 6
REPORT_SIZE = 30
EVENT_DATA_LEN_MAX = REPORT_SIZE - 6

MOD_FIELD_POS = 4
MOD_BROADCAST = 0xf

OPT_FIELD_POS = 0
OPT_FIELD_MAX_OPT_CNT = 0xf

OPT_BROADCAST_MAX_MOD_ID = 0x0

OPT_MODULE_DEV_DESCR = 0x0

POLL_INTERVAL_DEFAULT = 0.02
POLL_RETRY_COUNT = 200

END_OF_TRANSFER_CHAR = '\n'


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

class NrfHidDevice():
    def __init__(self, board_name, vid, pid, dongle_pid):
        self.name = board_name
        self.vid = vid
        self.pid = pid
        self.dev_ptr = None
        self.dev_config = None

        direct_devs = NrfHidDevice._open_devices(vid, pid)
        dongle_devs = []

        if dongle_pid is not None:
            dongle_devs = NrfHidDevice._open_devices(vid, dongle_pid)

        devs = direct_devs + dongle_devs

        for d in devs:
            if self.dev_ptr is None:
                board_name = NrfHidDevice._discover_board_name(d, pid)

                if board_name is not None:
                    config = NrfHidDevice._discover_device_config(d, pid)
                else:
                    config = None

                if config is not None:
                    self.dev_config = config
                    self.dev_ptr = d
                    print("Device board name is {}".format(board_name))
                else:
                    d.close()
            else:
                d.close()

    @staticmethod
    def _open_devices(vid, pid):
        devs = []
        try:
            devlist = hid.enumerate(vid=vid, pid=pid)
            for d in devlist:
                dev = hid.Device(path=d['path'])
                devs.append(dev)

        except hid.HIDException:
            pass
        except Exception as e:
            logging.error('Unknown exception: {}'.format(e))

        return devs

    @staticmethod
    def _create_set_report(recipient, event_id, event_data):
        """ Function creating a report in order to set a specified configuration
            value. """

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

    @staticmethod
    def _create_fetch_report(recipient, event_id):
        """ Function for creating a report which requests fetching of
            a configuration value from a device. """

        assert isinstance(recipient, int)
        assert isinstance(event_id, int)

        status = ConfigStatus.FETCH
        report = struct.pack('<BHBBB', REPORT_ID, recipient, event_id, status, 0)

        assert len(report) <= REPORT_SIZE
        report += b'\0' * (REPORT_SIZE - len(report))

        return report

    @staticmethod
    def _exchange_feature_report(dev, recipient, event_id, event_data, is_fetch,
                                 poll_interval=POLL_INTERVAL_DEFAULT):
        if is_fetch:
            data = NrfHidDevice._create_fetch_report(recipient, event_id)
        else:
            data = NrfHidDevice._create_set_report(recipient, event_id, event_data)

        try:
            dev.send_feature_report(data)
        except Exception:
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
                              '\tresponse: recipient {}, event_id {}'.format(recipient,
                                                                             event_id,
                                                                             response.recipient,
                                                                             response.event_id))
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

        return success, fetched_data

    @staticmethod
    def _fetch_max_mod_id(dev, recipient):
        event_id = (MOD_BROADCAST << MOD_FIELD_POS) | \
                   (OPT_BROADCAST_MAX_MOD_ID << OPT_FIELD_POS)
        event_data = struct.pack('<B', 0)

        success = NrfHidDevice._exchange_feature_report(dev, recipient,
                                                        event_id, event_data,
                                                        False)
        if not success:
            return False, None

        success, fetched_data = NrfHidDevice._exchange_feature_report(dev, recipient,
                                                                      event_id, None,
                                                                      True)
        if not success or not fetched_data:
            return False, None

        max_mod_id = ord(fetched_data.decode('utf-8'))

        return success, max_mod_id

    @staticmethod
    def _fetch_next_option(dev, recipient, module_id):
        event_id = (module_id << MOD_FIELD_POS) | (OPT_MODULE_DEV_DESCR << OPT_FIELD_POS)

        success, fetched_data = NrfHidDevice._exchange_feature_report(dev, recipient,
                                                                      event_id, None,
                                                                      True)
        if not success or not fetched_data:
            return False, None

        opt_name = fetched_data.decode('utf-8').replace(chr(0x00), '')

        return success, opt_name

    @staticmethod
    def _get_event_id(module_name, option_name, device_config):
        module_id = device_config[module_name]['id']
        option_id = device_config[module_name]['options'][option_name]['id']

        return (module_id << MOD_FIELD_POS) | (option_id << OPT_FIELD_POS)

    @staticmethod
    def _discover_module_config(dev, recipient, module_id):
        module_config = {}

        success, module_name = NrfHidDevice._fetch_next_option(dev, recipient,
                                                               module_id)
        if not success:
            return None, None

        module_config['id'] = module_id
        module_config['options'] = {}
        # First fetched option (with index 0) is module name
        opt_idx = 1

        while True:
            success, opt = NrfHidDevice._fetch_next_option(dev, recipient,
                                                           module_id)
            if not success:
                return None, None

            if opt[0] == END_OF_TRANSFER_CHAR:
                break

            if opt_idx > OPT_FIELD_MAX_OPT_CNT:
                print("Improper module description")
                return None, None

            module_config['options'][opt] = {
                'id' : opt_idx,
            }

            opt_idx += 1

        return module_name, module_config

    @staticmethod
    def _discover_device_config(dev, recipient):
        device_config = {}

        success, max_mod_id = NrfHidDevice._fetch_max_mod_id(dev, recipient)
        if not success or (max_mod_id is None):
            return None

        for i in range(0, max_mod_id + 1):
            module_name, module_config = NrfHidDevice._discover_module_config(dev, recipient, i)
            if (module_name is None) or (module_config is None):
                return None

            device_config[module_name] = module_config

        return device_config

    @staticmethod
    def _discover_board_name(dev, recipient):
        success, max_mod_id = NrfHidDevice._fetch_max_mod_id(dev, recipient)
        if not success:
            return None

        # Module with the highest index contains information about board name.
        # Discover only this module to recude discovery time.
        module_name, module_config = NrfHidDevice._discover_module_config(dev,
                                                                          recipient,
                                                                          max_mod_id)
        if (module_name is None) or (module_config is None):
            return None

        dev_cfg = {module_name : module_config}
        event_id = NrfHidDevice._get_event_id(module_name, 'board_name', dev_cfg)

        success, fetched_data = NrfHidDevice._exchange_feature_report(dev, recipient,
                                                                      event_id, None,
                                                                      True)

        board_name = fetched_data.decode('utf-8').replace(chr(0x00), '')

        return board_name

    def _config_operation(self, module_name, option_name, is_get, value, poll_interval):
        if not self.initialized():
            print("Device not found")

            if is_get:
                return False, None
            else:
                return False

        try:
            event_id = NrfHidDevice._get_event_id(module_name, option_name, self.dev_config)
        except KeyError:
            print("No module: {} or option: {}".format(module_name, option_name))
            if is_get:
                return False, None
            else:
                return False

        success, fetched_data = NrfHidDevice._exchange_feature_report(self.dev_ptr, self.pid,
                                                                      event_id, value,
                                                                      is_get, poll_interval)
        if is_get:
            return success, fetched_data
        else:
            return success

    def close_device(self):
        self.dev_ptr.close()
        self.dev_ptr = None
        self.dev_config = None

    def initialized(self):
        if (self.dev_ptr is None) or (self.dev_config is None):
            return False
        else:
            return True

    def get_device_config(self):
        if not self.initialized():
            print("Device is not initialized")
            return None

        res = {}

        for module in self.dev_config:
            res[module] = list(self.dev_config[module]['options'].keys())

        return res

    def config_get(self, module_name, option_name, poll_interval=POLL_INTERVAL_DEFAULT):
        return self._config_operation(module_name, option_name, True, None, poll_interval)

    def config_set(self, module_name, option_name, value, poll_interval=POLL_INTERVAL_DEFAULT):
        return self._config_operation(module_name, option_name, False, value, poll_interval)
