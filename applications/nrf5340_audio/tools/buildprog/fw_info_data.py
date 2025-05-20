#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Generate fw_info for B0N container from .config
"""

from intelhex import IntelHex

import argparse
import struct


def get_fw_info(input_hex, offset, magic_value, fw_version, fw_valid_val):
    # 0x0c start of fw_info_total_size
    # 0x10 start of flash_used
    # 0x14 start of fw_version
    # 0x18 the address of the start of the image
    # 0x1c the address of the boot point (vector table) of the firmware
    # 0x20 the address Value that can be modified to invalidate the firmware

    fw_info_bytes = magic_value
    for i in range(0xc, 0x14):
        fw_info_bytes += input_hex[offset + i].to_bytes(1, byteorder='little')
    fw_info_bytes += struct.pack('<I', fw_version)
    for i in range(0x18, 0x20):
        fw_info_bytes += input_hex[offset + i].to_bytes(1, byteorder='little')
    fw_info_bytes += fw_valid_val
    return fw_info_bytes


def inject_fw_info(input_file, offset, output_hex, magic_value, fw_version, fw_valid_val):
    ih = IntelHex(input_file)
    # OBJCOPY incorrectly inserts x86 specific records, remove the start_addr as it is wrong.
    ih.start_addr = None

    # Parse comma-separated string of uint32s into hex string. Each is encoded in little-endian byte order
    parsed_magic_value = b''.join(
        [struct.pack('<I', int(m, 0)) for m in magic_value.split(',')])
    # Parse string of uint32s into hex string. Each is encoded in little-endian byte order
    parsed_fw_valid_val = struct.pack('<I', fw_valid_val)
    parsed_fw_version = '0x%08X' % fw_version
    print(parsed_fw_version)
    fw_info_data = get_fw_info(input_hex=ih,
                               offset=offset,
                               magic_value=parsed_magic_value,
                               fw_version=fw_version,
                               fw_valid_val=parsed_fw_valid_val)
    fw_info_data_data_hex = IntelHex()

    fw_info_data_data_hex.frombytes(fw_info_data, offset)
    ih.merge(fw_info_data_data_hex, overlap='replace')
    ih.write_hex_file(output_hex)


def parse_args():
    parser = argparse.ArgumentParser(
        description='Inject fw info metadata at specified offset. Generate HEX file',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False)

    parser.add_argument('-i', '--input', required=True, type=argparse.FileType('r', encoding='UTF-8'),
                        help='Input hex file.')
    parser.add_argument('--offset', required=True, type=lambda x: int(x, 0),
                        help='Offset to store validation metadata at.', default=0x01008A00)
    parser.add_argument('-m', '--magic-value', required=True,
                        help='ASCII representation of magic value.')
    parser.add_argument('-v', '--fw-version', required=True, type=int,
                        help='Fw version.')
    parser.add_argument('-l', '--fw-valid-val', required=True, type=lambda x: int(x, 0),
                        help='ASCII representation of fw valid val.')
    parser.add_argument('-o', '--output-hex', required=False, default=None, type=argparse.FileType('w'),
                        help='.hex output file name. Default is to overwrite --input.')

    args = parser.parse_args()
    if args.output_hex is None:
        args.output_hex = args.input
    return args


def main():

    args = parse_args()

    inject_fw_info(input_file=args.input,
                   offset=args.offset,
                   output_hex=args.output_hex,
                   magic_value=args.magic_value,
                   fw_version=args.fw_version,
                   fw_valid_val=args.fw_valid_val)


if __name__ == '__main__':
    main()
