#!/usr/bin/env python3
#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
import re
import argparse


def func_names_from_header(in_file, out_file, exclude=None):
    with open(in_file) as f_in:
        content = f_in.read()

    with open(out_file, 'w') as f_out:
        # Regex match all function names in the header file
        # Tests for validating the regex in tests/unity/wrap
        x = re.findall(r"(?!^\s*static)^\s*(?:\w+[*\s]+)+(\w+?)\s*\([\w\s,*\.\[\]]*?\)\s*;",
                       content, re.M | re.S)
        exclude_regex = None if exclude is None else "^((" + ")|(".join(exclude) + "))"
        for item in x:
            if exclude_regex is None or re.match(exclude_regex, item) is None:
                f_out.write(item + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("-i", "--input", type=str,
                        help="input header file", required=True)
    parser.add_argument("-o", "--output", type=str,
                        help="output function list file", required=True)
    parser.add_argument("-e", "--exclude", type=str, action='append',
                        help="exclude functions matching pattern given")
    args = parser.parse_args()

    func_names_from_header(args.input, args.output, args.exclude)
