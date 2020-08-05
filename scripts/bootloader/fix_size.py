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

"""
See fw_info.h

uint32_t magic[MAGIC_LEN_WORDS];
uint32_t total_size;
uint32_t size;
uint32_t version;
uint32_t address;
uint32_t boot_address;
uint32_t valid;
uint32_t reserved[4];
uint32_t ext_api_num;
uint32_t ext_api_request_num;
const struct fw_info_ext_api ext_apis[];
"""

Fwinfo = namedtuple('Fwinfo',
                    'magic total_size size version address boot_address '
                    'valid reserved ext_api_num ext_api_request_num')
FW_INFO_FSTR = '<12sIIIIII16sII'
FW_INFO_SZ = struct.calcsize(FW_INFO_FSTR)


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument("-m", "--magic-value", required=True,
                        help="ASCII representation of magic value.")
    parser.add_argument("-i", "--in", "-in", required=False, dest="infile",
                        type=argparse.FileType('rb'), default=sys.stdin.buffer,
                        help="Sign the contents of the specified file instead of stdin.")
    parser.add_argument("-o", "--out", "-out", required=False, dest="outfile",
                        type=argparse.FileType('wb'), default=sys.stdout.buffer,
                        help="Write the signature to the specified file instead of stdout.")
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
    magic_addresses = [m.start()
                       for m in re.finditer(re.escape(magic), bs)]

    # All valid fw info addresses have the correct offset in the hex.
    # Use this info to filter out addresses that doesn't point to actual
    # fw info structs.
    info_addresses = [x for x in magic_addresses if x % args.offset == 0]

    if len(info_addresses) == 0:
        print("No fw_info found! Check '--magic-value' argument")
        sys.exit(-1)

    if len(info_addresses) > 1:
        infos = list()
        for addr in info_addresses:
            fields = struct.unpack(FW_INFO_FSTR, bs[addr:addr + FW_INFO_SZ])
            info = Fwinfo._make(fields)

            # Perform some checks to ensure that this is a fw_info struct
            if not [b != 0 for b in info.reserved]:
                print(f"Non zero reserved field found for info at {addr}")
                continue
            if info.address != info.boot_address:
                print(f"Non-matching boot-address and address field at {addr}")
                continue
            if info.valid != args.valid_val:
                print(f"Invalid valid field at {addr}")
                continue
            if info.total_size != FW_INFO_SZ:
                print(f"Invalid total_size at {addr}")
                continue

            infos.append(info)

        # Modify the size of all but the last image
        for i in infos[:-1]:
            # Create size pointing to end of last image
            new_size = (infos[-1].address - i.address) + infos[-1].size

            print(f"Changing size at address {i.address + args.offset} from"
                  f" {i.size} to {new_size} to point to validation info of "
                  f"last image in merged hex file.")

            i = i._replace(size=new_size)

            # Write the information back to the hex
            h.puts(i.address + args.offset, struct.pack(FW_INFO_FSTR, *i))

    h.tofile(args.outfile.name, format='hex')
