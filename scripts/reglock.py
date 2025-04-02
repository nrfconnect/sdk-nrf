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

SIZE_MAX_KB = 31

BOOTCONF_ADDR = 0x00FFD080

READ_ALLOWED = 0x01
WRITE_ALLOWED = 0x02
EXECUTE_ALLOWED = 0x04
SECURE = 0x08
OWNER_NONE = 0x00
OWNER_APP = 0x10
OWNER_KMU = 0x20
WRITEONCE = 0x10
LOCK = 0x20

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
    return parser.parse_args()


def main():
    args = parse_args()
    size = int(args.size, 16)
    if size % 1024:
        sys.exit("error: requested size not aligned to 1k")
    size = size // 1024
    if size > SIZE_MAX_KB:
        warnings.warn("warning: requested size too big; Setting to allowed maximum")
        size = SIZE_MAX_KB

    payload = bytearray([
        READ_ALLOWED | EXECUTE_ALLOWED | SECURE | OWNER_NONE,
        LOCK,
        size,
        0x0
        ])

    h = IntelHex()
    h.frombytes(bytes=payload, offset=BOOTCONF_ADDR)
    h.tofile(args.output, 'hex')

if __name__ == "__main__":
    main()
