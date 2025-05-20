#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
import argparse
import subprocess
from re import match

def dump_binary(binary):
    return ", ".join([f"0x{b:02X}" for b in binary])

def replace_manifest(source_file, binary_file):
    with open(source_file, 'r') as source:
        source_contents = source.readlines()

    with open(binary_file, 'rb') as binary:
        binary_contents = binary.read()

    collect = True
    dumped = False
    output_contents = ""
    for line in source_contents:
        if collect:
            output_contents += line
        if not dumped and match(r".*uint8_t.*\[.*\].*", line):
            collect = False
            output_contents += dump_binary(binary_contents)
            dumped = True
        if '};' in line:
            collect = True
            output_contents += "};\r\n"

    with open(source_file, 'w') as output_source:
        output_source.write(output_contents)

    subprocess.run(["clang-format", "-i", source_file])

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='replace_manifest',
        description='Replace const array contents with binary file, serialized as C array',
        allow_abbrev=False)
    parser.add_argument('source_file', help="C source file to be modified")
    parser.add_argument('binary_file', help="Binary file to be serialized")
    args = parser.parse_args()
    replace_manifest(args.source_file, args.binary_file)
