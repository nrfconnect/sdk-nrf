#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import struct
import zlib
import os
import time
import logging

import zipfile
from zipfile import ZipFile
import tempfile
import json

from NrfHidDevice import EVENT_DATA_LEN_MAX
import imgtool.image

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

    def get_flash_area_id(self):
        return self.flash_area_id

    def __str__(self):
        return ('Firmware info\n'
                '  FLASH area id: {}\n'
                '  Image length: {}\n'
                '  Version: {}.{}.{}.{}').format(self.flash_area_id, self.image_len,
                                                 self.ver_major, self.ver_minor,
                                                 self.ver_rev, self.ver_build_nr)


def b0_get_fwinfo_offset(dfu_bin):
    UPDATE_IMAGE_MAGIC_COMMON = 0x281ee6de
    UPDATE_IMAGE_MAGIC_FWINFO = 0x8fcebb4c
    UPDATE_IMAGE_MAGIC_COMPATIBILITY = 0x00003402
    UPDATE_IMAGE_HEADER_OFFSETS = (0x0000, 0x0200, 0x0400, 0x0800, 0x1000)

    fwinfo_offset = None
    img_file = None

    try:
        img_file = open(dfu_bin, 'rb')
        for offset in UPDATE_IMAGE_HEADER_OFFSETS:
            img_file.seek(offset)
            data_raw = img_file.read(4)
            magic_common = struct.unpack('<I', data_raw)[0]
            data_raw = img_file.read(4)
            magic_fwinfo = struct.unpack('<I', data_raw)[0]
            data_raw = img_file.read(4)
            magic_compat = struct.unpack('<I', data_raw)[0]
            if magic_common == UPDATE_IMAGE_MAGIC_COMMON and \
               magic_compat == UPDATE_IMAGE_MAGIC_COMPATIBILITY and \
               magic_fwinfo == UPDATE_IMAGE_MAGIC_FWINFO:
                fwinfo_offset = offset
                break
    except FileNotFoundError:
        print('Wrong file or file path')
    except Exception:
        print('Cannot process file')
    finally:
        if img_file is not None:
            img_file.close()

    return fwinfo_offset


def b0_is_dfu_file_correct(dfu_bin):
    if b0_get_fwinfo_offset(dfu_bin) is None:
        print('Invalid image format')
        return False

    return True


def b0_get_dfu_image_name(dfu_slot_id):
    return 'signed_by_b0_s{}_image.bin'.format(dfu_slot_id)


def b0_get_dfu_image_version(dfu_bin):
    fwinfo_offset = b0_get_fwinfo_offset(dfu_bin)
    if fwinfo_offset is None:
        return None

    version = None
    img_file = None

    try:
        img_file = open(dfu_bin, 'rb')
        img_file.seek(fwinfo_offset + 0x14)
        version_raw = img_file.read(4)
        version = (0, 0, 0, struct.unpack('<I', version_raw)[0])
    except FileNotFoundError:
        print('Wrong file or file path')
    except Exception:
        print('Cannot process file')
    finally:
        if img_file is not None:
            img_file.close()

    return version


def mcuboot_is_dfu_file_correct(dfu_bin):
    res, _ = imgtool.image.Image.verify(dfu_bin, None)

    if res != imgtool.image.VerifyResult.OK:
        print('DFU image is invalid')
        return False

    return True


def mcuboot_get_dfu_image_name(dfu_slot_id):
    return 'app_update.bin'


def mcuboot_get_dfu_image_version(dfu_bin):
    res, ver = imgtool.image.Image.verify(dfu_bin, None)

    if res != imgtool.image.VerifyResult.OK:
        print('Image in file is invalid')
        return None

    return ver


B0_API = {
    'get_dfu_image_version' : b0_get_dfu_image_version,
    'get_dfu_image_name' : b0_get_dfu_image_name,
    'is_dfu_file_correct' : b0_is_dfu_file_correct,
}

MCUBOOT_API = {
    'get_dfu_image_version' : mcuboot_get_dfu_image_version,
    'get_dfu_image_name' : mcuboot_get_dfu_image_name,
    'is_dfu_file_correct' : mcuboot_is_dfu_file_correct,
}

BOOTLOADER_APIS = {
    'MCUBOOT' : MCUBOOT_API,
    'B0' : B0_API,
}


class DfuImage:
    MANIFEST_FILE = "manifest.json"

    def __init__(self, dfu_package, dev_fwinfo, board_name):
        assert isinstance(dev_fwinfo, FwInfo)
        self.image_bin_path = None

        if not os.path.exists(dfu_package):
            print('File does not exist')
            return

        if not zipfile.is_zipfile(dfu_package):
            print('Invalid DFU package format')
            return

        self.temp_dir = tempfile.TemporaryDirectory(dir='.')

        with ZipFile(dfu_package, 'r') as zip_file:
            zip_file.extractall(self.temp_dir.name)

        try:
            with open(os.path.join(self.temp_dir.name, DfuImage.MANIFEST_FILE)) as f:
                manifest_str = f.read()
                self.bootloader_api = DfuImage._get_bootloader_api(manifest_str)
                self.manifest = json.loads(manifest_str)
        except Exception:
            self.bootloader_api = None
            self.manifest = None

        if self.bootloader_api is not None:
            assert 'get_dfu_image_version' in self.bootloader_api
            assert 'get_dfu_image_name' in self.bootloader_api
            assert 'is_dfu_file_correct' in self.bootloader_api

            self.image_bin_path = DfuImage._parse_dfu_image_bin_path(self.temp_dir.name,
                                                                     self.manifest,
                                                                     dev_fwinfo,
                                                                     board_name,
                                                                     self.bootloader_api)

    @staticmethod
    def _get_bootloader_api(manifest_str):
        bootloader_name = None
        for b in BOOTLOADER_APIS:
            if b in manifest_str:
                if bootloader_name is None:
                    bootloader_name = b
                else:
                    # There can be update only for one bootloader in the package.
                    assert False
                    return None

        if bootloader_name is not None:
            print('Update file is for {} bootloader'.format(bootloader_name))
            return BOOTLOADER_APIS[bootloader_name]
        else:
            return None

    @staticmethod
    def _is_dfu_file_correct(dfu_bin):
        if dfu_bin is None:
            return False

        if not os.path.isfile(dfu_bin):
            return False

        img_length = os.stat(dfu_bin).st_size

        if img_length <= 0:
            return False

        return True

    @staticmethod
    def _get_config_channel_board_name(board_name):
        return board_name.split('_')[0]

    @staticmethod
    def _parse_dfu_image_bin_path(dfu_folder, manifest, dev_fwinfo, board_name, bootloader_api):
        flash_area_id = dev_fwinfo.get_flash_area_id()
        if flash_area_id not in (0, 1):
            print('Invalid area ID in FW info')
            return None
        dfu_slot_id = 1 - flash_area_id

        dfu_image_name = bootloader_api['get_dfu_image_name'](dfu_slot_id)
        dfu_bin_path = None

        for f in manifest['files']:
            if f['file'] != dfu_image_name:
                continue

            if DfuImage._get_config_channel_board_name(f['board']) != board_name:
                continue

            dfu_bin_path = os.path.join(dfu_folder, f['file'])

            if not DfuImage._is_dfu_file_correct(dfu_bin_path):
                continue

            if not bootloader_api['is_dfu_file_correct'](dfu_bin_path):
                continue

            return dfu_bin_path

        return None

    def get_dfu_image_bin_path(self):
        return self.image_bin_path

    def get_dfu_image_version(self):
        if self.image_bin_path is None:
            return None
        else:
            return self.bootloader_api['get_dfu_image_version'](self.image_bin_path)

    def __del__(self):
        try:
            self.temp_dir.cleanup()
        except Exception:
            pass


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
        print('Please stop ongoing DFU and wait until device cleans memory.')
        return True

    return False
