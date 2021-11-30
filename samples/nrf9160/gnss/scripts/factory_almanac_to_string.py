#!/usr/bin/env python3
#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import sys

try:
    filename = sys.argv[1]
except IndexError:
    raise SystemExit(f"Usage: {sys.argv[0]} <filename>")

with open(filename, "rb") as f:
    i = 0
    byte = f.read(1)
    while byte:
        if i == 0:
            print('"', end = "")

        print(byte.hex(), end = "")

        if i == 31:
            print("\" \\")
            i = 0
        else:
            i = i + 1

        byte = f.read(1)

print('"')
