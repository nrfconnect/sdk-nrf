#!/usr/bin/env python3
#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Utility for manipulating DFU Multi Image packages.

DFU Multi Image package is a general-purpose update file consisting of
a CBOR-based header that describes contents of the package, followed by a number
of update components, such as firmware images for different MCU cores.

The CBOR header currently complies with the following format:
{
    "img": [
        {"id": 0, "size": 102400},
        {"id": 1, "size": 204800}
        ...
    ]
}

Usage examples:

Creating DFU Multi Image package:
./dfu_multi_image_tool.py create --image 0 app_update.bin --image 1 net_core_app_update.bin dfu_multi_image.bin

Showing DFU Multi Image package header:
./dfu_multi_image_tool.py show dfu_multi_image.bin
"""

import argparse
import cbor2
import struct
import os


# Buffer size used for file reads to ensure large files are not loaded into memory at once
READ_BUFFER_SIZE = 16 * 1024


def generate_header(image: list) -> bytes:
    """
    Generate DFU Multi Image package header
    """

    image_data = [{'id': int(id), 'size': os.path.getsize(path)} for id, path in image]
    header_data = {'img': image_data}
    header_cbor = cbor2.dumps(header_data)

    return struct.pack('<H', len(header_cbor)) + header_cbor


def parse_header(file: object) -> object:
    """
    Parse DFU Multi Image package header
    """

    header_fixed_size = struct.calcsize('<H')
    header_cbor_size, = struct.unpack('<H', file.read(header_fixed_size))
    header_cbor = file.read(header_cbor_size)

    return cbor2.loads(header_cbor)


def generate_image(images: list, output_file: str) -> None:
    """
    Generate DFU Multi Image package
    """

    with open(output_file, 'wb') as out_file:
        out_file.write(generate_header(images))

        for _, path in images:
            with open(path, 'rb') as file:
                while True:
                    chunk = file.read(READ_BUFFER_SIZE)
                    if not chunk:
                        break
                    out_file.write(chunk)


def show_header(input_file: str) -> None:
    """
    Parse and print DFU Multi Image package header
    """

    with open(input_file, 'rb') as file:
        header = parse_header(file)
        print('Images:')

        for image in header['img']:
            print(f'- Id: {image["id"]}')
            print(f'  Size: {image["size"]}')


def main():
    parser = argparse.ArgumentParser(description='DFU Multi Image tool',
                                     fromfile_prefix_chars='@',
                                     allow_abbrev=False)
    subcommands = parser.add_subparsers(dest='subcommand', title='valid subcommands')

    create_parser = subcommands.add_parser(
        'create', help='Create DFU Multi Image package')
    create_parser.add_argument(
        '-i', '--image',
        required=True, action='append', nargs=2, metavar=('id', 'path'),
        help='Image to be included in package')
    create_parser.add_argument(
        'output_file', help='Path to output package file')

    show_parser = subcommands.add_parser(
        'show', help='Show DFU Multi Image package header')
    show_parser.add_argument(
        'input_file', help='Path to package file')

    args = parser.parse_args()

    if args.subcommand == 'create':
        generate_image(args.image, args.output_file)
    elif args.subcommand == 'show':
        show_header(args.input_file)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
