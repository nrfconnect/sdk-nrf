#!/usr/bin/env python3
#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause


from subprocess import check_output
import re
import sys
import argparse


def get_ecdsa_signature(der, clength):
    # The der consists of a SEQUENCE with 2 INTEGERS (r and s)
    # The expected byte format of the file is
    #  0:        type(SEQ), len(SEQ)
    #  2:        type(INT), len(r)
    #  4:        r
    #  4+len(r): type(INT), len(s)
    #  6+len(r): s
    # Note that sometimes r or s is too short and must be
    # left-padded with zeros, or they might even be one byte too long with a
    # leading 0 byte.
    # clength is the expected length of r and s.
    # The following code parses the output of openssl asnparse which prints
    # the values in hex, together with human-readble metadata.

    # Disable pylint error as 'input' keyword has specific handling in 'check_output'
    # pylint: disable=unexpected-keyword-arg
    stdout = check_output(['openssl', 'asn1parse', '-inform', 'der'], input=der)
    sig = b''.join([bytes.fromhex(re.search(r'(?<=\:)([0-9A-F]+)', num)[0]).ljust(clength, b'\0') \
                    for num in re.findall(r'INTEGER *\:[0-9A-F]+', stdout.decode())])

    assert len(sig) == 2*clength
    return sig


def parse_args():
    parser = argparse.ArgumentParser(
        description='Decode DER format using OpenSSL.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False)

    parser.add_argument('-a', '--alg', required=True, choices=['rsa', 'ecdsa'],
                        help='Expected algorithm')
    parser.add_argument('-c', '--contents', required=True, choices=['signature'],
                        help='Expected contents')
    parser.add_argument('-i', '--in', '-in', required=False, dest='infile',
                        type=argparse.FileType('rb'), default=sys.stdin.buffer,
                        help='Parse the contents of the specified file instead of stdin.')

    args = parser.parse_args()

    return args


if __name__ == '__main__':
    args = parse_args()
    assert args.alg == 'ecdsa'  # Only ecdsa is currently supported.
    if args.alg == 'ecdsa':
        if args.contents == 'signature':
            sys.stdout.buffer.write(get_ecdsa_signature(args.infile.read(), 32))
