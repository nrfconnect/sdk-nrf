#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
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
    parser.add_argument('--start_time', help='Measurement start time[s]')
    parser.add_argument('--end_time', help='Measurement end time[s]')
    args = parser.parse_args()

    if args.log is not None:
        log_lvl_number = int(getattr(logging, args.log.upper(), None))
    else:
        log_lvl_number = logging.WARNING

    if args.start_time is None:
        args.start_time = 0
    else:
        args.start_time = float(args.start_time)

    if args.end_time is None:
        args.end_time = float('inf')
    else:
        args.end_time = float(args.end_time)

    sn = StatsNordic(args.event_csv, args.event_descr, log_lvl_number)
    sn.calculate_stats_preset1(args.start_time, args.end_time)

if __name__ == "__main__":
    main()
