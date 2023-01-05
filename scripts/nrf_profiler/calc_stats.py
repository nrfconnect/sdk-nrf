#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from stats_nordic import StatsNordic

import argparse
import logging


def main():
    parser = argparse.ArgumentParser(description='Calculating stats for events.',
                                     allow_abbrev=False)
    parser.add_argument('dataset_name', help='Name of dataset')
    parser.add_argument('--start_time', help='Measurement start time[s]')
    parser.add_argument('--end_time', help='Measurement end time[s]')
    parser.add_argument('--log', help='Log level')
    args = parser.parse_args()

    if args.log is not None:
        log_lvl_number = int(getattr(logging, args.log.upper(), None))
    else:
        log_lvl_number = logging.INFO

    if args.start_time is None:
        args.start_time = 0
    else:
        args.start_time = float(args.start_time)

    if args.end_time is None:
        args.end_time = float('inf')
    else:
        args.end_time = float(args.end_time)

    sn = StatsNordic(args.dataset_name + ".csv", args.dataset_name + ".json",
                     log_lvl_number)
    sn.calculate_stats_preset1(args.start_time, args.end_time)

if __name__ == "__main__":
    main()
