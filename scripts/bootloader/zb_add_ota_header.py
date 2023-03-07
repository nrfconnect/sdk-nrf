#!/usr/bin/env python3
#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import argparse
import os
import struct

OTA_HEADER_FILE_ID               = 0x0BEEF11E
OTA_HEADER_VERSION               = 0x0100
OTA_HEADER_LENGTH                = 4 + 2 + 2 + 2 + 2 + 2 + 4 + 2 + 32 + 4
OTA_HEADER_FIELD_CONTROL         = 0x00
OTA_HEADER_MANUFACTURER_WILDCARD = 0xFFFF
OTA_HEADER_IMAGE_TYPE_WILDCARD   = 0xFFFF
OTA_HEADER_STACK_VER_ZIGBEE_PRO  = 0x0002

OTA_HEADER_FIELD_CONTROL_BIT_MASK_HW_VER = (1 << 2)
OTA_HEADER_MIN_HW_VERSION_LENGTH         = 2
OTA_HEADER_MAX_HW_VERSION_LENGTH         = 2

OTA_SUBELEMENT_TYPE_IMAGE = 0x0000
OTA_SUBELEMENT_HEADER_LENGTH = 6

class OTA_file:

    def __init__(self,
                 file_version,
                 firmware_len,
                 firmware,
                 manufacturer_code = OTA_HEADER_MANUFACTURER_WILDCARD,
                 image_type = OTA_HEADER_IMAGE_TYPE_WILDCARD,
                 comment = '',
                 min_hw_version=None,
                 max_hw_version=None,
                 legacy_format=False):
        '''A constructor for the OTA file class, see Zigbee ZCL spec 11.4.2 (Zigbee Document 07-5123-06)
           see: https://zigbeealliance.org/wp-content/uploads/2019/12/07-5123-06-zigbee-cluster-library-specification.pdf
           (access verified as of 2020-07-16)
        '''

        if legacy_format is True:
            total_len = OTA_HEADER_LENGTH + firmware_len
        else:
            total_len = OTA_HEADER_LENGTH + OTA_SUBELEMENT_HEADER_LENGTH + firmware_len

        ota_header = OTA_header(OTA_HEADER_FILE_ID,
                                OTA_HEADER_VERSION,
                                OTA_HEADER_LENGTH,
                                OTA_HEADER_FIELD_CONTROL,
                                manufacturer_code,
                                image_type,
                                file_version,
                                OTA_HEADER_STACK_VER_ZIGBEE_PRO,
                                comment,
                                total_len,
                                min_hw_version,
                                max_hw_version)

        ota_firmware = OTA_subelement(OTA_SUBELEMENT_TYPE_IMAGE,
                                      firmware_len,
                                      firmware)

        if legacy_format is True:
            self.binary = ota_header.header + firmware
        else:
            self.binary = ota_header.header + ota_firmware.encode()
        self.filename = '-'.join(['{:04X}'.format(manufacturer_code),
                                  '{:04X}'.format(image_type),
                                  '{:08X}'.format(file_version),
                                  comment]) + '.zigbee'


class OTA_subelement:
    def __init__(self, type_id, length, firmware):
        self.__pack_format = '<HL'
        self._type = type_id
        self._length = length
        self._fw = firmware

    def encode(self):
        return struct.pack(self.__pack_format, self._type, self._length) + self._fw


class OTA_header:
    def __init__(self,
                 file_id,
                 header_version,
                 header_length,
                 header_field_control,
                 manufacturer_code,
                 image_type,
                 file_version,
                 zigbee_stack_version,
                 header_string,
                 total_image_size,
                 min_hw_version=None,
                 max_hw_version=None):

        self.__pack_format = '<LHHHHHLHc31sL'
        self.__header_length = header_length
        self.__total_size = total_image_size
        self.__field_control = header_field_control
        self.__additional_fields = []

        # If min and max hardware version are present add optional fields to the header
        if (isinstance(min_hw_version, int) and isinstance(max_hw_version, int)):
            self.__add_optional_fields((OTA_HEADER_MIN_HW_VERSION_LENGTH + OTA_HEADER_MAX_HW_VERSION_LENGTH),
                                       OTA_HEADER_FIELD_CONTROL_BIT_MASK_HW_VER,
                                       'HH',
                                       [min_hw_version, max_hw_version])

        self.__pack_args = [file_id,
                            header_version,
                            self.__header_length,
                            self.__field_control,
                            manufacturer_code,
                            image_type,
                            file_version,
                            zigbee_stack_version,
                            bytes([len(header_string)]),
                            bytes(header_string.encode('ascii')),
                            self.__total_size]
        self.__pack_args.extend(self.__additional_fields)

    @property
    def header(self):
        return struct.pack(self.__pack_format, *self.__pack_args)

    def __add_optional_fields(self, fields_length, field_control_bit_mask, fields_formatting, fields_values):
        self.__total_size    += fields_length
        self.__header_length += fields_length
        self.__field_control |= field_control_bit_mask
        self.__pack_format   += fields_formatting
        if type(fields_values) in (tuple, list):
            self.__additional_fields.extend(fields_values)
        else:
            self.__additional_fields.append(fields_values)

def convert_version_string_to_int(s):
    """Convert from semver string "1.2.3", to integer 1020003"""
    numbers = s.split('.')
    if len(numbers) != 3:
        raise ValueError('application-version-string parameter must be on the format x.y.z')
    js = [0x100*0x10000, 0x10000, 1]
    return sum([js[i] * int(numbers[i]) for i in range(3)])

def hex2int(x):
    """Convert hex to int."""
    return int(x, 0)

def parse_args():
    parser = argparse.ArgumentParser(
        description='Append Zigbee OTA header to BIN file',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False)

    parser.add_argument('--application', required=True,
                        help='The application firmware file.')
    parser.add_argument('--application-version-string', required=True,
                        help="The application version string, e.g. '2.7.31'. Will be converted to an integer, e.g. 20731.")
    parser.add_argument('--zigbee-manufacturer-id', required=False, default=OTA_HEADER_MANUFACTURER_WILDCARD, type=hex2int,
                        help='Manufacturer ID to be used in Zigbee OTA header.')
    parser.add_argument('--zigbee-image-type', required=False, default=OTA_HEADER_IMAGE_TYPE_WILDCARD, type=hex2int,
                        help='Image type to be used in Zigbee OTA header.')
    parser.add_argument('--zigbee-comment', required=False, default='', nargs='?', const='',
                        help='Firmware comment to be used in Zigbee OTA header.')
    parser.add_argument('--zigbee-ota-min-hw-version', required=False, type=hex2int, nargs='?',
                        help='The zigbee OTA minimum hw version of Zigbee OTA Client.')
    parser.add_argument('--zigbee-ota-max-hw-version', required=False, type=hex2int, nargs='?',
                        help='The zigbee OTA maximum hw version of Zigbee OTA Client.')
    parser.add_argument('--out-directory', required=False, default='', nargs='?',
                        help='The zigbee OTA maximum hw version of Zigbee OTA Client.')
    parser.add_argument('--legacy', required=False, action='store_true',
                        help='Generate OTA image using legacy format.')
    return parser.parse_args()


def main():
    args = parse_args()

    app_file = args.application
    app_version_int = convert_version_string_to_int(args.application_version_string)
    zb_manufacturer_id = args.zigbee_manufacturer_id
    zb_image_type = args.zigbee_image_type
    zb_comment = args.zigbee_comment
    out_dir = args.out_directory

    if any(ord(char) > 127 for char in zb_comment): # Check if all the characters belong to the ASCII range
        print('Warning: Non-ASCII characters in the comment are not allowed. Discarding comment.')
        zb_comment = ''
    elif len(zb_comment) > 30:
        print('Warning: truncating the comment to 30 bytes.')
        zb_comment = zb_comment[:30]

    if args.zigbee_ota_min_hw_version is not None and args.zigbee_ota_min_hw_version > 0xFFFF:
        raise ValueError('zigbee-ota-min-hw-version parameter exceeds 2-byte long integer.')
    zb_ota_min_hw_version = args.zigbee_ota_min_hw_version

    if args.zigbee_ota_max_hw_version is not None and args.zigbee_ota_max_hw_version > 0xFFFF:
        raise ValueError('zigbee-ota-max-hw-version parameter exceeds 2-byte long integer.')
    zb_ota_max_hw_version = args.zigbee_ota_max_hw_version

    # Warn user if minimal/maximum zigbee ota hardware version are not correct:
    #   * only one of them is given
    #   * minimum version is higher than maximum version
    #   * hw_version is inside the range specified by minimum and maximum hardware version
    if isinstance(zb_ota_min_hw_version, int) != isinstance(zb_ota_max_hw_version, int):
        print('Warning: min/max zigbee ota hardware version is missing. Discarding min/max hardware version.')
    elif isinstance(zb_ota_min_hw_version, int):
        if zb_ota_min_hw_version > zb_ota_max_hw_version:
            raise ValueError('Warning: zigbee-ota-min-hw-version is higher than zigbee-ota-max-hw-version.')

    with open(app_file, 'rb') as file:
        bin_file = file.read()

    zigbee_ota_file = OTA_file(app_version_int,
                                os.path.getsize(app_file),
                                bytes(bin_file),
                                zb_manufacturer_id,
                                zb_image_type,
                                zb_comment,
                                zb_ota_min_hw_version,
                                zb_ota_max_hw_version,
                                args.legacy)

    with open(os.path.join(out_dir, zigbee_ota_file.filename), 'wb') as f:
        f.write(zigbee_ota_file.binary)

    print(f'Zigbee update created at {zigbee_ota_file.filename}')


if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        print(e)
        exit(1)
