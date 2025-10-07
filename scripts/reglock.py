# Copyright 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
NRF54L15 BOOTCONF generator.
"""

from intelhex import IntelHex
import argparse
import sys
import warnings
import struct

BOOTCONF_ADDR = 0x00FFD080

READ_ALLOWED = 0x01
WRITE_ALLOWED = 0x02
EXECUTE_ALLOWED = 0x04
SECURE = 0x08
WRITEONCE = 0x10
LOCK = 0x2000

SIZE_OFFSET = 16


supported_socs = [
    "nrf54l05",
    "nrf54l10",
    "nrf54l15",
    "nrf54lm20a",
    "nrf54lv10a",
    "nrf54ls05b",
]

def get_max_size_kb(soc):
    if soc in ["nrf54l05", "nrf54l10", "nrf54l15"]:
        return 31
    elif soc in ["nrf54lm20a", "nrf54lv10a"]:
        return 127
    elif soc in ["nrf54ls05b"]:
        return 1023
    else:
        sys.exit("error: unsupported SoC")

def get_bootconf_reg_32bit_value(soc, size):
    value = READ_ALLOWED | EXECUTE_ALLOWED | LOCK

    if soc not in ["nrf54ls05b"]:
        value |= SECURE

    size_kb = size // 1024
    max_size_kb = get_max_size_kb(soc)

    if size_kb > max_size_kb:
        warnings.warn("warning: requested size too big; Setting to allowed maximum")
        size_kb = max_size_kb

    value |= size_kb << SIZE_OFFSET
    return value

def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate RRAMC's BOOTCONF configuration hex",
        formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)

    parser.add_argument("-o", "--output", required=False, default="b0_lock.hex",
                        type=argparse.FileType('w', encoding='UTF-8'),
                        help="Output file name.")
    parser.add_argument("-s", "--size", required=False, default="0x7C00",
                        type=lambda x: hex(int(x, 0)),
                        help="Size to lock.")
    parser.add_argument("--soc", required=True,
                        type=str,
                        help="SoC for which to generate the bootconf.")
    return parser.parse_args()


def main():
    args = parse_args()
    size = int(args.size, 16)
    if size % 1024:
        sys.exit("error: requested size not aligned to 1k")
    if args.soc not in supported_socs:
        sys.exit("error: unsupported SoC")

    reg_value = get_bootconf_reg_32bit_value(args.soc, size)

    payload = struct.pack('<I', reg_value)

    h = IntelHex()
    h.frombytes(bytes=payload, offset=BOOTCONF_ADDR)
    h.tofile(args.output, 'hex')

if __name__ == "__main__":
    main()
