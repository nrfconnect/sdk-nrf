#!/usr/bin/env python3
#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic


import sys
import argparse
import struct
import re
from intelhex import IntelHex
from collections import namedtuple

# See fw_info.h
Fwinfo = namedtuple('Fwinfo',
                    'magic total_size size version address boot_address '
                    'valid reserved ext_api_num ext_api_request_num')
FW_INFO_FSTR = '<12sIIIIII16sII'
FW_INFO_SZ = struct.calcsize(FW_INFO_FSTR)


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument("-m", "--magic-value", required=True,
                        help="ASCII representation of magic value.")
    parser.add_argument("-i", "--in", "-in", required=True, dest="infile",
                        type=argparse.FileType('rb'),
                        help="Input hex file to update 'size' fields in.")
    parser.add_argument("-o", "--out", "-out", required=False, dest="outfile",
                        type=argparse.FileType('wb'),
                        help="Output hex file to write result to.")
    parser.add_argument("--offset", required=True, type=lambda x: int(x, 0),
                        help="Offset of fw_info inside image.")
    parser.add_argument("--valid-val", required=True, type=lambda x: int(x, 0),
                        help="Value of 'valid' field.")

    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    h = IntelHex(args.infile.name)
    bs = h.tobinstr()
    magic = b''.join([struct.pack("<I", int(m, 0))
                      for m in args.magic_value.split(",")])
    magic_indexes = [m.start()
                     for m in re.finditer(re.escape(magic), bs)]

    if len(magic_indexes) == 0:
        print("No fw_info found! Check '--magic-value' argument")
        sys.exit(-1)

    if len(magic_indexes) > 1:
        infos = list()
        hex_file_base = h.segments()[0][0]
        for idx in magic_indexes:
            fields = struct.unpack(FW_INFO_FSTR, bs[idx:idx + FW_INFO_SZ])
            info = Fwinfo._make(fields)

            # Perform some checks to ensure that this is a fw_info struct
            if hex_file_base + idx != info.address + args.offset:
                # No print here since this will occur for all non-struct
                # mentions of the magic value in the hex file
                continue
            if any([b != 0 for b in info.reserved]):
                print(f"Non zero reserved field found for info at {idx}")
                continue
            if info.valid != args.valid_val:
                print(f"Invalid valid field at {idx}")
                continue
            if not infos and info.address != hex_file_base:
                print(f"Address field of first fw_info does not point to"
                      f"start of hex file")
                continue

            infos.append(info)

        if len(infos) < 2:
            print("Could not find 2 or more valid fw_info structs")
            sys.exit(-1)

        # Fix the size in the first image, as this is the only one read by b0

        # Create size pointing to end of last image
        new_size = (infos[-1].address - infos[0].address) + infos[-1].size

        print(f"Size at 0x{(infos[0].address + args.offset):x} changed from"
              f" 0x{infos[0].size:x} to 0x{new_size:x} to point to validation "
              f"info.")

        print(f"BEFORE: {infos[0]}")
        infos[0] = infos[0]._replace(size=new_size)
        print(f"AFTER: {infos[0]}")

        # Write the information back to the hex
        h.puts(infos[0].address + args.offset, struct.pack(FW_INFO_FSTR, *infos[0]))

    h.tofile(args.outfile.name, format='hex')
