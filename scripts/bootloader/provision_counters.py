#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause


from intelhex import IntelHex

import argparse
import struct

def main():
    args = parse_args()

    provision_counters = add_hw_counters(
        args.num_counter_slots_version,
        args.num_counter_slots_mcuboot
    )

    assert len(provision_counters) <= args.size, """Provisioning data doesn't fit.
Reduce the number of counter slots and try again."""

    ih = IntelHex()

    ih.frombytes(provision_counters, offset=args.address)

    ih.write_hex_file(args.output)

def parse_args():
    parser = argparse.ArgumentParser(
        description='Generate provisioning hex file.',
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--address', type=lambda x: int(x, 0),
                        required=True, help='PM_PROVISION_COUNTERS_ADDRESS')
    parser.add_argument('--size', type=lambda x: int(x, 0),
                        required=True, help='PM_PROVISION_COUNTERS_SIZE')
    parser.add_argument('-o', '--output', required=False, default='provision.hex',
                        help='Output file name.')
    parser.add_argument('--num-counter-slots-version', required=False, type=int, default=0,
                        help='Number of monotonic counter slots for the version number.')
    parser.add_argument('--num-counter-slots-mcuboot', required=False, type=int, default=0,
                        help='Number of monotonic counter slots for the MCUBOOT counter.')
    return parser.parse_args()


def add_hw_counters(nsib_counter_slots, mcuboot_counters_slots):
    num_counters = min(mcuboot_counters_slots, 1) + min(nsib_counter_slots, 1)

    assert num_counters > 0, "Both slots were 0"

    provision_counters = struct.pack('H', 1) # Type "counter collection"
    provision_counters += struct.pack('H', num_counters)

    slots = [nsib_counter_slots, mcuboot_counters_slots]

    for i in range(2):
        if slots[i]:
            description = i + 1 # 1-indexed
            provision_counters += struct.pack('H', description)
            provision_counters += struct.pack('H', slots[i])
            provision_counters += bytes(2 * slots[i] * [0xFF])

    return provision_counters

if __name__ == '__main__':
    main()
