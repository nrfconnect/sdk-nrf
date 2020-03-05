#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import struct
import zlib
import os
import imgtool.image as img
import time
import logging

from NrfHidDevice import EVENT_DATA_LEN_MAX

FLASH_PAGE_SIZE = 4096

DFU_SYNC_RETRIES = 3
DFU_SYNC_INTERVAL = 1


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
                '  Version: {}.{}.{}.{}').format(self.flash_area_id, self.image_len,
                                                 self.ver_major, self.ver_minor,
                                                 self.ver_rev, self.ver_build_nr)


def fwinfo(dev):
    success, fetched_data = dev.config_get('dfu', 'fwinfo')

    if success and fetched_data:
        fw_info = FwInfo(fetched_data)
        return fw_info
    else:
        return None


def fwreboot(dev):
    success, fetched_data = dev.config_get('dfu', 'reboot')

    if (not success) or (fetched_data is None):
        return False, False

    fmt = '<?'
    assert struct.calcsize(fmt) <= EVENT_DATA_LEN_MAX
    rebooted = struct.unpack(fmt, fetched_data)

    if success and rebooted:
        logging.debug('Firmware rebooted')
    else:
        logging.debug('FW reboot request failed')

    return success, rebooted


def dfu_sync(dev):
    success, fetched_data = dev.config_get('dfu', 'sync')

    if (not success) or (fetched_data is None):
        return None

    fmt = '<BIII'
    assert struct.calcsize(fmt) <= EVENT_DATA_LEN_MAX

    if (fetched_data is None) or (len(fetched_data) < struct.calcsize(fmt)):
        return None

    return struct.unpack(fmt, fetched_data)


def dfu_start(dev, img_length, img_csum, offset):
    # Start DFU operation at selected offset.
    # It can happen that device will reject this request - this will be
    # verified by dfu sync at data exchange.
    event_data = struct.pack('<III', img_length, img_csum, offset)

    success = dev.config_set('dfu', 'start', event_data)

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


def dfu_sync_wait(dev, is_active):
    if is_active:
        dfu_state = 0x01
    else:
        dfu_state = 0x00

    for _ in range(DFU_SYNC_RETRIES):
        dfu_info = dfu_sync(dev)

        if dfu_info is not None:
            if dfu_info[0] != dfu_state:
                # DFU may be transiting its state. This can happen when previous
                # interrupted DFU operation is timing out. Sleep to allow it
                # to settle the state.
                time.sleep(DFU_SYNC_INTERVAL)
            else:
                break

    return dfu_info


def dfu_transfer(dev, dfu_image, progress_callback):
    img_length = os.stat(dfu_image).st_size
    dfu_info = dfu_sync_wait(dev, False)

    if not is_dfu_image_correct(dfu_image):
        return False

    if is_dfu_operation_pending(dfu_info):
        return False

    img_csum = file_crc(dfu_image)

    if not img_csum:
        return False

    offset = get_dfu_operation_offset(dfu_image, dfu_info, img_csum)

    success = dfu_start(dev, img_length, img_csum, offset)

    if not success:
        print('Cannot start DFU operation')
        return False

    img_file = open(dfu_image, 'rb')
    img_file.seek(offset)

    try:
        offset, success = send_chunk(dev, img_csum, img_file, img_length, offset, success, progress_callback)
    except Exception:
        success = False

    img_file.close()
    print('')

    if success:
        print('DFU transfer completed')
        success = False

        dfu_info = dfu_sync_wait(dev, False)

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


def send_chunk(dev, img_csum, img_file, img_length, offset, success, progress_callback):
    while offset < img_length:
        if offset % FLASH_PAGE_SIZE == 0:
            # Sync DFU state at regular intervals to ensure everything
            # is all right.
            success = False
            dfu_info = dfu_sync(dev)

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

        success = dev.config_set('dfu', 'data', chunk_data)

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
