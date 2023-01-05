#!/usr/bin/env python3
#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import argparse
import yaml
import platform
from os import path


def get_size_str(size):
    return f'{int(size/1024):d}kB' if size >= 1024 else f'{size:d}B'


def print_region(domain, region, size, pm_config):
    # Prepare colors (color code taken from size_report)
    bcolors_ansi = {
        'HEADER'    : '\033[95m',
        'OKBLUE'    : '\033[94m',
        'OKGREEN'   : '\033[92m',
        'WARNING'   : '\033[93m',
        'FAIL'      : '\033[91m',
        'ENDC'      : '\033[0m',
        'BOLD'      : '\033[1m',
        'UNDERLINE' : '\033[4m'
    }
    if platform.system() == 'Windows':
        # Set all color codes to empty string on Windows
        bcolors = dict.fromkeys(bcolors_ansi, '')
    else:
        bcolors = bcolors_ansi

    # Print header
    print(f"{bcolors['OKBLUE']} {domain} {region} ({hex(size)} - {get_size_str(size)}): {bcolors['ENDC']}")

    # Sort partitions three times:
    #  1. On whether they are a container (has a 'span'), containers first.
    #  2. On size, descending.
    #  3. On address, ascending.
    sorted_pm_config = sorted(pm_config.keys(), key=lambda x: int('span' in pm_config[x]), reverse=True)
    sorted_pm_config = sorted(sorted_pm_config, key=lambda x: pm_config[x]['size'], reverse=True)
    sorted_pm_config = sorted(sorted_pm_config, key=lambda x: pm_config[x]['address'])

    # Create text lines
    lines = []
    endspan = 0
    for name in sorted_pm_config:
        lines.append('{}{}: {} ({} - {}){}'.format(
                '| '+bcolors['WARNING'] if 'span' not in pm_config[name] else '+---'+bcolors['OKBLUE'],
                hex(pm_config[name]['address']),
                name,
                hex(pm_config[name]['size']),
                get_size_str(pm_config[name]['size']),
                bcolors['ENDC']))
        if name == sorted_pm_config[-1]:
            continue
        if 'span' in pm_config[name]:
            endspan = pm_config[name]['end_address']
        if pm_config[name]['end_address'] == endspan and ('span' not in pm_config[name]) \
                and 'span' not in pm_config[sorted_pm_config[sorted_pm_config.index(name)+1]]:
            lines.append('+' + bcolors['OKBLUE'] + bcolors['ENDC'])
    maxlen = max(map(len, lines)) + 1

    # Add top and bottom of frame. Add dummy color so alignment always works.
    top_bottom = '+' + bcolors['OKBLUE'] + bcolors['ENDC']
    lines = [top_bottom, *lines, top_bottom]

    # Print left-justified, framed lines
    list(map(lambda s: print(s.ljust(maxlen, ' ') + '|' if s[0] != '+' else s.ljust(maxlen, '-') + '+'), lines))
    print('')


def parse_args():
    parser = argparse.ArgumentParser(
        description='Parse given Partition Manager output YAML file and print a pretty report',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False)
    parser.add_argument('-i', '--input', required=True, type=str, nargs='+',
                        help='Path to the domain specific YAML files from Partition Manager')

    args = parser.parse_args()

    return args


def main():
    args = parse_args()

    if not args.input:
        raise RuntimeError('No input files provided')

    for i in args.input:
        fn = path.basename(i)
        if '_' in fn:
            domain_name = fn[fn.index('partitions_') + len('partitions_'):fn.index('.yml')]
        else:
            domain_name = ''
        items = []
        with open(i, 'r') as f:
            items = yaml.safe_load(f).items()
        regions = set(val['region'] for _, val in items)
        for r in sorted(regions):
            pm_config_primary = {k: v for k, v in items if v['region'] == r}
            min_address = min((part['address'] for part in pm_config_primary.values()
                               if 'address' in part))
            max_address = max((part['address'] + part['size'] for part in pm_config_primary.values()
                               if 'address' in part))
            print_region(domain_name, r, max_address - min_address, pm_config_primary)


if __name__ == '__main__':
    main()
