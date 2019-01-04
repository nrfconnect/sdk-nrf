# Copyright (c) 2019 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

from stats_nordic import StatsNordic

import sys
import argparse
import logging

def main():
    parser = argparse.ArgumentParser(description='Calculating stats for events.')
    parser.add_argument('event_csv', help='.csv file to read raw events data')
    parser.add_argument('event_descr', help='.json file to read events descriptions')
    parser.add_argument('--log', help='Log level')
    args = parser.parse_args()

    if args.log is not None:
        log_lvl_number = int(getattr(logging, args.log.upper(), None))
    else:
        log_lvl_number = logging.WARNING

    sn = StatsNordic(args.event_csv, args.event_descr, log_lvl_number)
    sn.calculate_stats_preset1()

if __name__ == "__main__":
    main()
