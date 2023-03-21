#!/usr/bin/env python3
#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from intelhex import IntelHex as ih
from struct import pack
from subprocess import run
from argparse import ArgumentParser

key_nrf53 = [
    0xc5, 0xa8, 0x08, 0xeb, 0xe3, 0x1e, 0xa5, 0xb4,
    0xe9, 0x44, 0x1a, 0x76, 0x45, 0x58, 0xd8, 0x8b,
    0x40, 0x26, 0x33, 0xa8, 0xcd, 0x2d, 0x51, 0x67,
    0x8d, 0xef, 0x00, 0x24, 0x30, 0x52, 0xd7, 0x3d]

key_nrf91 = [
    0x19, 0x9a, 0xe3, 0xc7, 0x9d, 0xd0, 0x16, 0x8c,
    0x3e, 0xee, 0xa8, 0x46, 0xea, 0x4e, 0xdc, 0x6e]

dest_nrf91 = 0x50841400
dest_nrf53_1 = 0x50845400
dest_nrf53_2 = 0x50845410
perm = 0xFFFF0004 # PUSH

def write_kmu(key, perm, target_addr, slot, snr):

    target_addr_offset = 0x00FF8400 + slot * 0x8
    perm_offset = 0x00FF8404 + slot * 0x8
    keyslot_offset = 0x00FF8800 + slot * 0x10
    select_offset = 0x50039500

    ih_nrf = ih()
    ih_nrf.frombytes(pack("<I", target_addr), offset = target_addr_offset)
    ih_nrf.frombytes(key, offset = keyslot_offset)

    run(["nrfjprog", "--snr", snr, "--memwr", str(select_offset), "--val", str(slot+1)]).check_returncode()

    run(["nrfjprog", "--snr", snr, "--memwr", str(target_addr_offset), "--val", str(target_addr)]).check_returncode()
    # The KMU slots are 128 bits and we need to write them in words since the --program argument
    # does not work for the KMU region with the nrfjprog
    for word_offset in range(0,16,4):
        run(["nrfjprog", "--snr", snr,
             "--memwr", str(keyslot_offset + word_offset),
             "--val", str(int.from_bytes(key[word_offset:word_offset+4], 'little'))]).check_returncode()

    run(["nrfjprog", "--snr", snr, "--memwr", str(perm_offset), "--val", str(perm)]).check_returncode()
    run(["nrfjprog", "--snr", snr, "--memwr", str(select_offset), "--val", str(0)]).check_returncode()



def parse_args():
    parser = ArgumentParser(description='''Write the test vector key to the KMU.''')

    parser.add_argument("--snr", required=True, help="SEGGER serial number of the board")
    parser.add_argument("--family", required=True, choices=("NRF53", "NRF91"), help="nRF chip family.")

    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()

    run(["nrfjprog", "--snr", args.snr, "-e"]).check_returncode()

    if args.family == "NRF91":
        write_kmu(bytes(key_nrf91), perm, dest_nrf91, 4, args.snr)
    else:
        write_kmu(bytes(key_nrf53[:16]), perm, dest_nrf53_1, 4, args.snr)
        write_kmu(bytes(key_nrf53[16:]), perm, dest_nrf53_2, 5, args.snr)
