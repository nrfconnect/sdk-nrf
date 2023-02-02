#!/usr/bin/env python3
#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Utility for programming nRF53 application into the external flash
"""

import argparse
from intelhex import IntelHex
import os
import subprocess
import sys
import tempfile

# Base address of the memory-mapped QSPI flash on nRF53 SoCs
NRF53_XIP_REGION_BASE = 0x10000000


def die(msg):
    sys.stderr.write(msg + '\n')
    sys.exit(1)


def program(app_data, app_offset, net_data, net_offset, snr):
    with tempfile.TemporaryDirectory() as outdir:
        dest_file = os.path.join(outdir, 'external.hex')

        ihex = IntelHex()
        ihex.putsz(NRF53_XIP_REGION_BASE + app_offset, app_data)
        ihex.putsz(NRF53_XIP_REGION_BASE + net_offset, net_data)
        ihex.write_hex_file(dest_file)

        subprocess.check_call(['nrfjprog', '-s', snr,
                               '--program', dest_file,
                               '--reset', '--verify'])


def select_board_snr():
    boards = subprocess.check_output(['nrfjprog', '-i']).decode('ascii').splitlines()

    if not boards:
        die('There are no boards connected.')

    if len(boards) == 1:
        return boards[0]

    print('There are multiple boards connected.')
    print('\n'.join(f'{idx}. {snr}' for (idx, snr) in enumerate(boards, 1)))

    while True:
        idx = input(f'Please select one with desired serial number(1-{len(boards)}): ')

        try:
            return boards[int(idx) - 1]
        except Exception:
            pass

def main():
    # Type for an integer of arbitrary base
    def any_base_int(s): return int(s, 0)

    parser = argparse.ArgumentParser(description="Program firmware into external flash",
                                     allow_abbrev=False)
    parser.add_argument('--app', required=True, type=argparse.FileType('rb'),
                        help="Application core image in MCUboot format")
    parser.add_argument('--app-offset', type=any_base_int, required=True,
                        help='Offset of target partition for application core image')
    parser.add_argument('--app-size', type=any_base_int, required=True,
                        help='Size of target partition for application core image')
    parser.add_argument('--net', required=True, type=argparse.FileType('rb'),
                        help="Network core image in MCUboot format")
    parser.add_argument('--net-offset', type=any_base_int, required=True,
                        help='Offset of target partition for network core image')
    parser.add_argument('--net-size', type=any_base_int, required=True,
                        help='Size of target partition for network core image')
    args = parser.parse_args()

    app_data = args.app.read()
    net_data = args.net.read()

    if len(app_data) > args.app_size:
        die(f'Application core image exceeds partition size: {len(app_data)} > {args.app_size}')

    if len(net_data) > args.net_size:
        die(f'Network core image exceeds partition size: {len(net_data)} > {args.net_size}')

    snr = select_board_snr()
    program(app_data, args.app_offset, net_data, args.net_offset, snr)


if __name__ == "__main__":
    main()
