#!/usr/bin/env python3
#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
import re
import argparse


def func_names_from_header(in_file, out_file):
    with open(in_file) as f_in:
        content = f_in.read()

    with open(out_file, 'w') as f_out:
        # Regex match all function names in the header file
        x = re.findall(r"^\s*(?:\w+[*\s]+)+(\w+?)\(.*?\);",
                       content, re.M | re.S)
        for item in x:
            f_out.write(item + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--input", type=str,
                        help="input header file", required=True)
    parser.add_argument("-o", "--output", type=str,
                        help="output function list file", required=True)
    args = parser.parse_args()

    func_names_from_header(args.input, args.output)
