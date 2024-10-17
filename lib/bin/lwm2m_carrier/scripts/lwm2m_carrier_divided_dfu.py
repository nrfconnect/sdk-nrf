#!/usr/bin/env python3
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import argparse
import os
import struct
import errno

HEADER_LENGTH = 42
HEADER_MAGIC = 0x424ad2dc

class chunk_header:
    def __init__(self, number, is_last, offset, version_str):
        self.__number = number
        self.__is_last = is_last
        self.__offset = offset
        self.__version_str = version_str.encode('utf-8')
        self.__pack_format = '<IB?I32s'

    def encode(self):
        """
        Encode the header, which consists of:
			- Header magic (4 bytes).
            - File number (1 byte).
            - Flag indicating whether the file is the last in the sequence (1 byte).
            - Offset of the application update data relative to the full sequence (4 bytes).
            - Application version string (32 bytes).
        """
        return struct.pack(self.__pack_format, HEADER_MAGIC, self.__number, self.__is_last, self.__offset, self.__version_str)

def split(image, max_chunk_size):
    """
    Split the provided image into chunks of size of up to max_chunk_size bytes.
    """
    chunks = []

    with open(image, 'rb') as file:
        while True:
            chunk = file.read(max_chunk_size)
            if not chunk:
                break
            chunks.append(chunk)

    return chunks

def parse_args():
    parser = argparse.ArgumentParser(
        description='Split the application update image into proprietary LwM2M Carrier divided DFU files',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False)

    parser.add_argument('-i', '--input-file', required=True, help='The application update file to be divided.')
    parser.add_argument('-v', '--version-str', required=True, help='The application version string to be included in the header (maximum 32 bytes including the null termination).')
    parser.add_argument('-s', '--max-file-size', default=100, type=int, help='(Optional) The maximum size of a divided DFU file in KB.')
    parser.add_argument('-o', '--output-dir', required=True, help='The directory to store the divided DFU files into.')

    return parser.parse_args()

def main():
    # Parse arguments.
    args = parse_args()

    # Validate the length of the version string.
    if len(args.version_str) > 31:
        raise OSError(errno.EINVAL, "Input version string is too long")

    input_file = args.input_file
    version_str = args.version_str
    max_chunk_size = (args.max_file_size * 1024) - HEADER_LENGTH
    output_dir = args.output_dir

    # Split the application update image into chunks of max_chunk_size bytes, which excludes
    # the length of the header from the maximum size of a divided DFU file.
    chunks = split(input_file, max_chunk_size)
    total_chunks = len(chunks)

    # Variable to track the offset of each chunk within the whole image.
    offset = 0

    print("Generating LwM2M Carrier divided DFU files...")

    # Append headers to each chunk and store in the output directory.
    for i, chunk in enumerate(chunks):
        # Create the chunk header (1-indexed numbering).
        header = chunk_header(i + 1, (i == total_chunks - 1), offset, version_str)

        # Increase the offset (image chunk only, no header included).
        offset += len(chunk)

        # Append the header to the chunk.
        chunk = header.encode() + chunk

        # Generate file name of each chunk.
        chunk_version_str = version_str + '_{0:03d}.bin'.format(i + 1)

        # Write the chunk into the output directory.
        with open(os.path.join(output_dir, chunk_version_str), 'wb') as chunk_file:
            chunk_file.write(chunk)
            chunk_file.close()
            print("Encoded %6d bytes to %s" % (len(chunk), chunk_version_str))

    print("LwM2M Carrier divided DFU files generated")

if __name__ == '__main__':
    try:
        main()
    except Exception:
        print("Failed to generate LwM2M Carrier divided DFU files")
        exit(1)
