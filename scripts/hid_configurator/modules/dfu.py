#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

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

DFU_SYNC_INTERVAL = 1

class DFUInfo:
    _DFU_STATE_INACTIVE = 0x00
    _DFU_STATE_ACTIVE   = 0x01
    _DFU_STATE_STORING  = 0x02
    _DFU_STATE_CLEANING = 0x03

    _DFUSTATE2STR = {
        _DFU_STATE_INACTIVE: "Inactive",
        _DFU_STATE_ACTIVE:   "Active",
        _DFU_STATE_STORING:  "Storing",
        _DFU_STATE_CLEANING: "Cleaning"
    }

    def __init__(self, fetched_data):
        fmt = '<BIIIH'
        assert struct.calcsize(fmt) <= EVENT_DATA_LEN_MAX
        assert struct.calcsize(fmt) == len(fetched_data)
        vals = struct.unpack(fmt, fetched_data)
        self.dfu_state = vals[0]
        self.img_length = vals[1]
        self.img_csum = vals[2]
        self.offset = vals[3]
        self.sync_buffer_size = vals[4]

    def get_img_length(self):
        return self.img_length

    def get_img_csum(self):
        return self.img_csum

    def get_offset(self):
        return self.offset

    def get_sync_buffer_size(self):
        return self.sync_buffer_size

    def is_started(self):
        return self.dfu_state in (self._DFU_STATE_ACTIVE, self._DFU_STATE_STORING)

    def is_storing(self):
        return self.dfu_state == self._DFU_STATE_STORING

    def is_cleaning(self):
        return self.dfu_state == self._DFU_STATE_CLEANING

    def is_busy(self):
        return self.dfu_state != self._DFU_STATE_INACTIVE

    def __str__(self):
        return ('DFU synchronization data\n'
                '  state: {}\n'
                '  Image length: {}\n'
                '  Image checksum: {}\n'
                '  Offset: {}\n'
                '  Sync buffer size: {}').format(self._DFUSTATE2STR[self.dfu_state], self.img_length,
                                                 self.img_csum, self.offset, self.sync_buffer_size)


class FwInfo:
    def __init__(self, fetched_data):
        fmt = '<BIBBHI'
        assert struct.calcsize(fmt) <= EVENT_DATA_LEN_MAX
        assert struct.calcsize(fmt) == len(fetched_data)
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


class DevInfo:
    def __init__(self, fetched_data):
        fmt = '<HH'
        assert struct.calcsize(fmt) < len(fetched_data)
        self.vid, self.pid = struct.unpack(fmt, fetched_data[:struct.calcsize(fmt)])

        # The remaining data represents device generation
        generation = fetched_data[struct.calcsize(fmt):]
        self.generation = generation.decode('utf-8').replace(chr(0x00), '')

    def __str__(self):
        return ('Device info\n'
                '  Vendor ID: {}\n'
                '  Product ID: {}\n'
                '  Generation: {}').format(hex(self.vid), hex(self.pid), self.generation)


def b0_get_fwinfo_offset(dfu_bin):
    UPDATE_IMAGE_MAGIC_COMMON = 0x281ee6de
    UPDATE_IMAGE_MAGIC_FWINFO = 0x8fcebb4c
    UPDATE_IMAGE_MAGIC_COMPATIBILITY = (
                                        0x00003402,   # nRF52
                                        0x00003502    # nRF53
                                        )
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
               magic_compat in UPDATE_IMAGE_MAGIC_COMPATIBILITY and \
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


def b0_get_dfu_image_bootloader_var():
    return 'B0'


def mcuboot_common_is_dfu_file_correct(dfu_bin):
    res, _, _ = imgtool.image.Image.verify(dfu_bin, None)

    if res != imgtool.image.VerifyResult.OK:
        print('DFU image is invalid')
        return False

    return True


def mcuboot_common_get_dfu_image_version(dfu_bin):
    res, ver, _ = imgtool.image.Image.verify(dfu_bin, None)

    if res != imgtool.image.VerifyResult.OK:
        print('Image in file is invalid')
        return None

    return ver


def mcuboot_get_dfu_image_name(dfu_slot_id):
    return 'app_update.bin'


def mcuboot_get_dfu_image_bootloader_var():
    return 'MCUBOOT'


def mcuboot_xip_get_dfu_image_name(dfu_slot_id):
    assert dfu_slot_id in (0, 1)

    if dfu_slot_id == 0:
        return 'app_update.bin'
    else:
        return 'mcuboot_secondary_app_update.bin'


def mcuboot_xip_get_dfu_image_bootloader_var():
    return 'MCUBOOT+XIP'


B0_API = {
    'get_dfu_image_version' : b0_get_dfu_image_version,
    'get_dfu_image_bootloader_var' : b0_get_dfu_image_bootloader_var,
    'get_dfu_image_name' : b0_get_dfu_image_name,
    'is_dfu_file_correct' : b0_is_dfu_file_correct,
}

MCUBOOT_API = {
    'get_dfu_image_version' : mcuboot_common_get_dfu_image_version,
    'get_dfu_image_bootloader_var' : mcuboot_get_dfu_image_bootloader_var,
    'get_dfu_image_name' : mcuboot_get_dfu_image_name,
    'is_dfu_file_correct' : mcuboot_common_is_dfu_file_correct,
}

MCUBOOT_XIP_API = {
    'get_dfu_image_version' : mcuboot_common_get_dfu_image_version,
    'get_dfu_image_bootloader_var' : mcuboot_xip_get_dfu_image_bootloader_var,
    'get_dfu_image_name' : mcuboot_xip_get_dfu_image_name,
    'is_dfu_file_correct' : mcuboot_common_is_dfu_file_correct,
}

BOOTLOADER_APIS = {
    'MCUBOOT' : MCUBOOT_API,
    'MCUBOOT+XIP' : MCUBOOT_XIP_API,
    'B0' : B0_API,
}


class DfuImage:
    MANIFEST_FILE = "manifest.json"

    def __init__(self, dfu_package, dev_fwinfo, dev_board_name, dev_bootloader_variant):
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
                self.manifest = json.loads(manifest_str)
                self.bootloader_api = DfuImage._get_bootloader_api(self.manifest)
        except Exception:
            self.bootloader_api = None
            self.manifest = None

        if self.bootloader_api is not None:
            assert 'get_dfu_image_version' in self.bootloader_api
            assert 'get_dfu_image_bootloader_var' in self.bootloader_api
            assert 'get_dfu_image_name' in self.bootloader_api
            assert 'is_dfu_file_correct' in self.bootloader_api

            self.image_bin_path = DfuImage._parse_dfu_image_bin_path(self.temp_dir.name,
                                                                     self.manifest,
                                                                     dev_fwinfo,
                                                                     dev_board_name,
                                                                     dev_bootloader_variant,
                                                                     self.bootloader_api)

    @staticmethod
    def _get_bootloader_api(manifest_json):
        bootloader_name = None

        for f in manifest_json["files"]:
            VERSION_PREFIX = "version_"

            version_keys = tuple(filter(lambda x: x.startswith(VERSION_PREFIX), f.keys()))
            if len(version_keys) != 1:
                print("Invalid DFU zip manifest: improper version definition count")
                return None

            temp_bootloader_name = version_keys[0][len(VERSION_PREFIX):]
            if (bootloader_name is not None) and (temp_bootloader_name != bootloader_name):
                print("Invalid DFU zip manifest: update images for multiple bootloaders")
                return None

            bootloader_name = temp_bootloader_name

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
    def _parse_dfu_image_bin_path(dfu_folder, manifest, dev_fwinfo, dev_board_name, dev_bootloader_variant, bootloader_api):
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

            if DfuImage._get_config_channel_board_name(f['board']) != dev_board_name:
                continue

            dfu_bin_path = os.path.join(dfu_folder, f['file'])

            if not DfuImage._is_dfu_file_correct(dfu_bin_path):
                continue

            if not bootloader_api['is_dfu_file_correct'](dfu_bin_path):
                continue

            if (dev_bootloader_variant != bootloader_api['get_dfu_image_bootloader_var']()) and (dev_bootloader_variant is not None):
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

    def get_dfu_image_bootloader_var(self):
        return self.bootloader_api['get_dfu_image_bootloader_var']()

    def __del__(self):
        try:
            self.temp_dir.cleanup()
        except Exception:
            pass


def fwinfo(dev):
    dfu_module_name = dev.get_complete_module_name('dfu')
    if dfu_module_name:
        success, fetched_data = dev.config_get(dfu_module_name, 'fwinfo')
    else:
        print('Module DFU not found')
        return None

    if success and fetched_data:
        fw_info = FwInfo(fetched_data)
        return fw_info
    else:
        return None


def devinfo(dev):
    dev_info = None
    dfu_module_name = dev.get_complete_module_name('dfu')

    if dfu_module_name:
        success, fetched_data = dev.config_get(dfu_module_name, 'devinfo')
        if success and fetched_data:
            dev_info = DevInfo(fetched_data)

    return dev_info


def fwreboot(dev):
    dfu_module_name = dev.get_complete_module_name('dfu')
    if dfu_module_name:
        success, fetched_data = dev.config_get(dfu_module_name , 'reboot')
    else:
        print('Module DFU not found')
        return False, False

    if (not success) or (fetched_data is None):
        return False, False

    fmt = '<?'
    assert struct.calcsize(fmt) <= EVENT_DATA_LEN_MAX
    assert struct.calcsize(fmt) == len(fetched_data)
    rebooted = struct.unpack(fmt, fetched_data)

    if success and rebooted:
        logging.debug('Firmware rebooted')
    else:
        logging.debug('FW reboot request failed')

    return success, rebooted


def dfu_sync(dev):
    dfu_module_name = dev.get_complete_module_name('dfu')
    if dfu_module_name:
        success, fetched_data = dev.config_get(dfu_module_name , 'sync')
    else:
        print('Module DFU not found')
        return None

    if success and fetched_data:
        dfu_info = DFUInfo(fetched_data)
        return dfu_info
    else:
        return None


def dfu_start(dev, img_length, img_csum, offset):
    # Start DFU operation at selected offset.
    # It can happen that device will reject this request - this will be
    # verified by dfu sync at data exchange.
    event_data = struct.pack('<III', img_length, img_csum, offset)

    dfu_module_name = dev.get_complete_module_name('dfu')
    if dfu_module_name:
        success = dev.config_set(dfu_module_name , 'start', event_data)
    else:
        print('Module DFU not found')
        return False

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


def dfu_sync_wait_until_inactive(dev):
    state_shown = False

    while True:
        dfu_info = dfu_sync(dev)
        if dfu_info is None:
            break

        if dfu_info.is_cleaning() and not state_shown:
            print('Waiting for device to clean update slot')
            state_shown = True

        if dfu_info.is_busy():
            # DFU may be transiting its state. This can happen when previous
            # interrupted DFU operation is timing out. Sleep to allow it
            # to settle the state.
            time.sleep(DFU_SYNC_INTERVAL)
        else:
            break

    return dfu_info


def dfu_transfer(dev, dfu_image, progress_callback):
    img_length = os.stat(dfu_image).st_size
    img_csum = file_crc(dfu_image)
    if not img_csum:
        return False

    while True:
        dfu_info = dfu_sync_wait_until_inactive(dev)

        if is_dfu_operation_pending(dfu_info):
            return False

        offset = get_dfu_operation_offset(dfu_image, dfu_info, img_csum)

        success = dfu_start(dev, img_length, img_csum, offset)
        if not success:
            print('Cannot start DFU operation')
            return False

        dfu_info = dfu_sync(dev)
        if dfu_info.is_started():
            break

    img_file = open(dfu_image, 'rb')
    img_file.seek(offset)

    try:
        success, offset = send_chunks(dev, img_csum, img_file, img_length, offset, dfu_info.get_sync_buffer_size(), progress_callback)
    except Exception:
        success = False

    img_file.close()
    print('')

    if success:
        success = False

        dfu_info = dfu_sync_wait_until_inactive(dev)

        if dfu_info is None:
            print('Lost communication with the device')
        else:
            if dfu_info.is_busy():
                print('Device holds DFU active')
            elif dfu_info.get_offset() != offset:
                print('Offset {} does not match device info {}'.format(offset, dfu_info))
            else:
                success = True

    return success


def send_chunks(dev, img_csum, img_file, img_length, offset, sync_buffer_size, progress_callback):
    def dfu_checkpoint(img_csum, img_length, offset):
        # Sync DFU state at regular intervals to ensure everything
        # is all right.
        store_retry = 0
        sleep_time = 0.3
        while True:
            dfu_info = dfu_sync(dev)
            if dfu_info is None:
                print('Lost communication with the device')
                return False
            if (dfu_info.get_img_length() != img_length) or (dfu_info.get_img_csum() != img_csum):
                print('Invalid sync information {}'.format(dfu_info))
                return False
            if (not dfu_info.is_busy()) and (dfu_info.get_offset() != img_length):
                print('DFU interrupted by device')
                return False
            if dfu_info.is_storing():
                # DFU store in progress - retry
                store_retry += 1
                if store_retry % 8 == 0: sleep_time += sleep_time
                if sleep_time > DFU_SYNC_INTERVAL: sleep_time = DFU_SYNC_INTERVAL
                time.sleep(sleep_time)
                continue
            if (dfu_info.is_busy()) and (dfu_info.get_offset() != offset):
                print('Mismatching offset after synchronization {} != {}'.format(dfu_info.get_offset(), offset))
                return False
            return True

    next_checkpoint = offset + sync_buffer_size
    if next_checkpoint > img_length: next_checkpoint = img_length

    while offset < img_length:
        # Set current progress
        progress_callback(int(offset / img_length * 1000))

        # Read data from the file
        chunk_len = EVENT_DATA_LEN_MAX
        if next_checkpoint - offset < chunk_len:
            chunk_len = next_checkpoint - offset
        chunk_data = img_file.read(chunk_len)
        chunk_len = len(chunk_data)
        if chunk_len == 0:
            break

        # Send data to the device
        logging.debug('Send DFU request: offset {}, size {}'.format(offset, chunk_len))
        dfu_module_name = dev.get_complete_module_name('dfu')
        if dfu_module_name:
            success = dev.config_set(dfu_module_name , 'data', chunk_data)
        else:
            print('Module DFU not found')
            return False, offset
        if not success:
            print('Lost communication with the device')
            break

        # Progress checkpoint
        offset += chunk_len
        if offset >= next_checkpoint:
            success = dfu_checkpoint(img_csum, img_length, offset)
            if not success: break
            next_checkpoint += sync_buffer_size
            if next_checkpoint > img_length: next_checkpoint = img_length

    return success, offset


def get_dfu_operation_offset(dfu_image, dfu_info, img_csum):
    # Check if the previously interrupted DFU operation can be resumed.
    img_length = os.stat(dfu_image).st_size

    if not img_csum:
        return None

    if (dfu_info.get_img_length() == img_length) and (dfu_info.get_img_csum() == img_csum) and (dfu_info.get_offset() <= img_length):
        print('Resume DFU at {}'.format(dfu_info.get_offset()))
        offset = dfu_info.get_offset()
    else:
        offset = 0

    return offset


def is_dfu_operation_pending(dfu_info):
    # Check there is no other DFU operation.
    if dfu_info is None:
        print('Cannot start DFU, device not responding')
        return True

    if dfu_info.is_busy():
        print('Cannot start DFU. DFU in progress or memory is not clean.')
        print('Please stop ongoing DFU and wait until device cleans memory.')
        return True

    return False
