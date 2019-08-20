#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import collections
import logging
import signal
import sys
import time
import zlib


import hid
import imgtool.image as img
import os
import random
import struct
from enum import IntEnum

ConfigOption = collections.namedtuple('ConfigOption', 'range event_id help')

REPORT_ID = 5
REPORT_SIZE = 30
EVENT_DATA_LEN_MAX = REPORT_SIZE - 6

TYPE_FIELD_POS = 0
GROUP_FIELD_POS = 6
EVENT_GROUP_SETUP = 0x1
EVENT_GROUP_DFU = 0x2
EVENT_GROUP_LED_STREAM = 0x3

MOD_FIELD_POS = 3
SETUP_MODULE_SENSOR = 0x1

OPT_FIELD_POS = 0
SENSOR_OPT_CPI = 0x0
SENSOR_OPT_DOWNSHIFT_RUN = 0x1
SENSOR_OPT_DOWNSHIFT_REST1 = 0x2
SENSOR_OPT_DOWNSHIFT_REST2 = 0x3

DFU_START = 0x0
DFU_DATA = 0x1
DFU_SYNC = 0x2
DFU_REBOOT = 0x3
DFU_IMGINFO = 0x4

LED_STREAM_DATA = 0x0

FLASH_PAGE_SIZE = 4096

POLL_INTERVAL = 0.005
POLL_RETRY_COUNT = 200

DFU_SYNC_RETRIES = 3
DFU_SYNC_INTERVAL = 1

PMW3360_OPTIONS = {
    'downshift_run':    ConfigOption((10,   2550),   SENSOR_OPT_DOWNSHIFT_RUN,   'Run to Rest 1 switch time [ms]'),
    'downshift_rest1':  ConfigOption((320,  81600),  SENSOR_OPT_DOWNSHIFT_REST1, 'Rest 1 to Rest 2 switch time [ms]'),
    'downshift_rest2':  ConfigOption((3200, 816000), SENSOR_OPT_DOWNSHIFT_REST2, 'Rest 2 to Rest 3 switch time [ms]'),
    'cpi':              ConfigOption((100,  12000),  SENSOR_OPT_CPI,             'CPI resolution'),
}

PAW3212_OPTIONS = {
    'sleep1_timeout':  ConfigOption((32,    512),    SENSOR_OPT_DOWNSHIFT_RUN,   'Sleep 1 switch time [ms]'),
    'sleep2_timeout':  ConfigOption((20480, 327680), SENSOR_OPT_DOWNSHIFT_REST1, 'Sleep 2 switch time [ms]'),
    'sleep3_timeout':  ConfigOption((20480, 327680), SENSOR_OPT_DOWNSHIFT_REST2, 'Sleep 3 switch time [ms]'),
    'cpi':             ConfigOption((0,     2394),   SENSOR_OPT_CPI,             'CPI resolution'),
}

PCA20041_CONFIG = {
    'sensor' : {
        'id' : SETUP_MODULE_SENSOR,
        'options' : PMW3360_OPTIONS
    }
}

PCA20044_CONFIG = {
    'sensor' : {
        'id' : SETUP_MODULE_SENSOR,
        'options' : PAW3212_OPTIONS
    }
}

PCA20045_CONFIG = {
    'sensor' : {
        'id' : SETUP_MODULE_SENSOR,
        'options' : PAW3212_OPTIONS
    }
}

DEVICE = {
    'desktop_mouse_nrf52832' : {
        'vid' : 0x1915,
        'pid' : 0x52DA,
        'config' : PCA20044_CONFIG,
        'stream' : False,
    },
    'desktop_mouse_nrf52810' : {
        'vid' : 0x1915,
        'pid' : 0x52DB,
        'config' : PCA20045_CONFIG,
        'stream' : False,
    },
    'gaming_mouse' : {
        'vid' : 0x1915,
        'pid' : 0x52DE,
        'config' : PCA20041_CONFIG,
        'stream' : True,
    },
    'keyboard' : {
        'vid' : 0x1915,
        'pid' : 0x52DD,
        'config' : None,
        'stream' : False,
    },
    'dongle' : {
        'vid' : 0x1915,
        'pid' : 0x52DC,
        'config' : None,
        'stream' : False,
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


def create_set_report(recipient, event_id, event_data):
    """ Function creating a report in order to set a specified configuration
        value.
        Recipient is a device product ID. """

    assert type(recipient) == int
    assert type(event_id) == int
    if event_data:
        assert type(event_data) == bytes
        event_data_len = len(event_data)

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


def exchange_feature_report(dev, recipient, event_id, event_data, is_fetch):
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
        time.sleep(POLL_INTERVAL)

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

    if not check_range(config_value, value_range):
        print('Failed. Config value for {} must be in range {}'.format(config_name, value_range))
        return False

    event_data = struct.pack('<I', config_value)

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

    if success and fetched_data:
        val = int.from_bytes(fetched_data, byteorder='little')
    else:
        val = None

    return success, val


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
        print('Cannot start DFU, already in progress')
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
    def __init__(self,
                 r=random.randint(0, 255),
                 g=random.randint(0, 255),
                 b=random.randint(0, 255),
                 substep_count=100, substep_time=20):
        self.r = r
        self.g = g
        self.b = b
        self.substep_count = substep_count
        self.substep_time = substep_time


def led_send_single_step(dev, recipient, step, led_id):
    event_id = (EVENT_GROUP_LED_STREAM << GROUP_FIELD_POS) | (LED_STREAM_DATA << TYPE_FIELD_POS)

    event_data = struct.pack('<BBBHHB', step.r, step.g, step.b, step.substep_count, step.substep_time, led_id)

    try:
        success = exchange_feature_report(dev, recipient, event_id, event_data, False)
    except:
        success = False

    return success


def fetch_free_steps_buffor_info(dev, recipient, led_id):
    event_id = (EVENT_GROUP_LED_STREAM << GROUP_FIELD_POS) | (led_id << MOD_FIELD_POS)

    try:
        success, fetched_data = exchange_feature_report(dev, recipient, event_id, None, True)
    except:
        success = False
        fetched_data = None

    if not success:
        print('Fetch failed')
        return None, None

    fmt = '<B?'
    assert struct.calcsize(fmt) <= EVENT_DATA_LEN_MAX

    if (fetched_data is None) or (len(fetched_data) < struct.calcsize(fmt)):
        print('Fetched data corrupted')
        return None, None

    return struct.unpack(fmt, fetched_data)


STREAM_HZ = 30
substep_count = int(1000/STREAM_HZ)
count = 1
first_time = 0
last_time = None
free = 0


class Color:
    def __init__(self, r=0, g=0, b=0):
        self.r = r
        self.g = g
        self.b = b


def led_send_hz(dev, recipient, color, led_id):
    global substep_count
    global last_time
    global count
    global first_time
    global free

    if free is not None:
        if STREAM_HZ < 5:
            print('Frequency too low')
            return

        was_sleeping = False
        cur_time = time.time()

        if last_time is not None:
            freq = 1/((cur_time - first_time)/count)
            cur_dif = cur_time - last_time
            if cur_dif < 1/STREAM_HZ:
                time.sleep((1/STREAM_HZ-cur_dif))
                was_sleeping = True
        else:
            cur_dif = 0
            first_time = cur_time
            freq = 0

        count +=1

        if free < 1:
            substep_count -= 1/8
        elif free > 2:
            substep_count += 1/8
        elif free > 3:
            substep_count += 1/4

        if substep_count <1:
            substep_count = 1

        step = Step(color.r, color.g, color.b, int(substep_count), 3)
        led_send_single_step(dev, recipient, step, led_id)
        last_time = time.time()
        print('free: {}, substeps: {}, was_sleeping {}, cur_dif {}, freq {}'.format(free, substep_count, was_sleeping, cur_dif, freq))
    free, _ = fetch_free_steps_buffor_info(dev, recipient, led_id)


interrupted = False


def send_continuous_led_stream(dev, recipient, led_id):
    def signal_handler(signal, frame):
        global interrupted
        interrupted = True

    signal.signal(signal.SIGINT, signal_handler)

    while not interrupted:
        free, _ = fetch_free_steps_buffor_info(dev, recipient, led_id)
        if free is None:
            return
        if free > 0:
            step = Step(
                random.randint(0, 255),
                random.randint(0, 255),
                random.randint(0, 255),
            )
            led_send_single_step(dev, recipient, step, led_id)


if __name__ == '__main__':
    print("Please run configurator_cli.py or gui.py to start application")
