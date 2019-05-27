#!/usr/bin/env python3
#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

import argparse
import yaml
import platform
import sys

def print_region(region, size, pm_config):
    print("Partition Manager report")

    # Prepare colors (color code taken from size_report)
    bcolors_ansi = {
        "HEADER"    : '\033[95m',
        "OKBLUE"    : '\033[94m',
        "OKGREEN"   : '\033[92m',
        "WARNING"   : '\033[93m',
        "FAIL"      : '\033[91m',
        "ENDC"      : '\033[0m',
        "BOLD"      : '\033[1m',
        "UNDERLINE" : '\033[4m'
    }
    if platform.system() == "Windows":
        # Set all color codes to empty string on Windows
        bcolors = dict.fromkeys(bcolors_ansi, '')
    else:
        bcolors = bcolors_ansi

    # Print header
    print(bcolors["OKBLUE"] + "%s (0x%x):" % (region, size) + bcolors["ENDC"])

    # Sort partitions three times:
    #  1. On whether they are a container (has a 'span'), containers first.
    #  2. On size, descending.
    #  3. On address, ascending.
    sorted_pm_config = sorted(pm_config.keys(), key=lambda x: int('span' in pm_config[x]), reverse=True)
    sorted_pm_config = sorted(sorted_pm_config, key=lambda x: pm_config[x]['size'], reverse=True)
    sorted_pm_config = sorted(sorted_pm_config, key=lambda x: pm_config[x]['address'])

    # Create text lines
    lines = ["%s0x%x: %s (0x%x)%s" %
                ("| "+bcolors["WARNING"] if 'span' not in pm_config[name] else "+---"+bcolors["OKBLUE"],
                pm_config[name]['address'],
                name,
                pm_config[name]['size'],
                bcolors["ENDC"])
            for name in sorted_pm_config]
    maxlen = max(map(len, lines)) + 1

    # Add top and bottom of frame. Add dummy color so alignment always works.
    top_bottom = "+" + bcolors["OKBLUE"] + bcolors["ENDC"]
    lines = [top_bottom, *lines, top_bottom]

    # Print left-justified, framed lines
    list(map(lambda s: print('%s' % s.ljust(maxlen, " ") + '|' if s[0] != '+' else s.ljust(maxlen, "-") + '+'), lines))


def parse_args():
    parser = argparse.ArgumentParser(
        description='Parse given Partition Manager output YAML file and print a pretty report',
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-i", "--input", required=True, type=str,
                        help="Path to the YAML file from Partition Manager")
    parser.add_argument("-q", "--quiet", required=False, action='store_true',
                        help="Don't print anything")

    args = parser.parse_args()

    return args


def main():
    args = parse_args()

    if args.quiet:
        sys.exit(0)
    with open(args.input, 'r') as f:
        pm_config = yaml.safe_load(f)
    min_address = min((part['address'] for part in pm_config.values() if 'address' in part))
    max_address = max((part['address'] + part['size'] for part in pm_config.values() if 'address' in part))
    print_region('FLASH', max_address - min_address, pm_config)


if __name__ == "__main__":
    main()
