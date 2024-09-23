#!/usr/bin/env python3
#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
import re
import argparse

# Tests for validating the regexes are located tests/unity/wrap

def header_prepare(in_file, out_file, out_wrap_file):
    with open(in_file) as f_in:
        content = f_in.read()

    # remove comments
    c_comments_pattern = re.compile(
        r'/\*([^*]|[\r\n]|(\*+([^*/]|[\r\n])))*\*+/',
        re.DOTALL | re.MULTILINE
    )

    content = c_comments_pattern.sub(r"", content)

    cpp_comments_pattern = re.compile(
        r'(?!.*\".*)\/\/.*$',
        re.MULTILINE
    )

    content = cpp_comments_pattern.sub(r"", content)

    # remove inline syscalls
    static_inline_pattern = re.compile(
        r'(?:__deprecated\s+)?(?:static\s+inline\s+|inline\s+static\s+|static\s+ALWAYS_INLINE\s+|__STATIC_INLINE\s+)'
        r'((?:\w+[*\s]+)+z_impl_\w+?\(.*?\))\n\{.+?\n\}',
        re.M | re.S)
    content = static_inline_pattern.sub(r"", content)

    # change static inline functions to normal function declaration
    static_inline_pattern = re.compile(
        r'(?:__deprecated\s+)?(?:static\s+inline\s+|inline\s+static\s+|static\s+ALWAYS_INLINE\s+|__STATIC_INLINE\s+)'
        r'((?:\w+[*\s]+)+\w+?\(.*?\))\n\{.+?\n\}',
        re.M | re.S)
    content = static_inline_pattern.sub(r"\1;", content)

    # remove syscall include
    syscall_pattern = re.compile(r"#include <zephyr/syscalls/\w+?.h>", re.M | re.S)
    content = syscall_pattern.sub(r"", content)

    syscall_decl_pattern = re.compile(
        r'__syscall\s+',
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

    # Prepare file with functions prefixed with __cmock_ that will be used for
    # mock generation.
    func_pattern = re.compile(
        r"^\s*((?:\w+[*\s]+)+)(\w+?\s*\([\w\s,*\.\[\]]*?\)\s*;)", re.M)
    content2 = func_pattern.sub(r"\n\1__cmock_\2", content)

    with open(out_wrap_file, 'w') as f_wrap:
        f_wrap.write(content2)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("-i", "--input", type=str,
                        help="input header file", required=True)
    parser.add_argument("-o", "--output", type=str,
                        help="stripped header file to be included in the test",
                        required=True)
    parser.add_argument("-w", "--wrap", type=str,
                        help='header with __cmock_-prefixed functions for'
                        'mock generation', required=True)
    args = parser.parse_args()

    header_prepare(args.input, args.output, args.wrap)
