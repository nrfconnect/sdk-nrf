#!/usr/bin/env python3
#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
import re
import argparse


def header_prepare(in_file, out_file, out_wrap_file):
    with open(in_file) as f_in:
        content = f_in.read()

    # remove comments
    comments_pattern = re.compile(
        r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
        re.DOTALL | re.MULTILINE
    )
    content = comments_pattern.sub(r"", content)

    # change static inline functions to normal function declaration
    static_inline_pattern = re.compile(
        r'(?:__deprecated\s+)?(?:static\s+inline\s+|static\s+ALWAYS_INLINE\s+|__STATIC_INLINE\s+)'
        r'((?:\w+[*\s]+)+\w+?\(.*?\))\n\{.+?\n\}',
        re.M | re.S)
    (content, static_inline_cnt) = static_inline_pattern.subn(r"\1;",
                                                              content)

    # remove syscall include
    syscall_pattern = re.compile(r"#include <syscalls/\w+?.h>", re.M | re.S)
    content = syscall_pattern.sub(r"", content)

    syscall_decl_pattern = re.compile(
        r'__syscall\s+(?:\w+[*\s]+)+(.+?;)',
        re.M | re.S)
    content = syscall_decl_pattern.sub("", content)

    # For now it handles extern function declaration but maybe extended later
    # if other cases are found.
    prefixed_func_decl_pattern = re.compile(
        r'extern\s+((?:\w+[*\s]+)+\w+?\(.*?\);)',
        re.M | re.S)
    content = prefixed_func_decl_pattern.sub(r"\1", content)

    with open(out_file, 'w') as f_out:
        f_out.write(content)

    # Prepare file with functions prefixed with __wrap_ that will be used for
    # mock generation.
    func_pattern = re.compile(
        r"^\s*((?:\w+[*\s]+)+)(\w+?\(.*?\);)", re.M | re.S)
    (content2, m2) = func_pattern.subn(r"\n\1__wrap_\2", content)

    with open(out_wrap_file, 'w') as f_wrap:
        f_wrap.write(content2)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--input", type=str,
                        help="input header file", required=True)
    parser.add_argument("-o", "--output", type=str,
                        help="stripped header file to be included in the test",
                        required=True)
    parser.add_argument("-w", "--wrap", type=str,
                        help='header with __wrap_-prefixed functions for'
                        'mock generation', required=True)
    args = parser.parse_args()

    header_prepare(args.input, args.output, args.wrap)
