# Copyright (c) 2018 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

from rtt_nordic_profiler_host import RttNordicProfilerHost
import sys
import argparse
import logging


def main():
    parser = argparse.ArgumentParser(
        description='Collecting data from Nordic profiler for given time and saving to files.')
    parser.add_argument('time', type=int, help='Time of collecting data')
    parser.add_argument('event_csv', help='.csv file to save collected events')
    parser.add_argument('event_descr', help='.json file to save events descriptions')
    parser.add_argument('--log', help='Log level')
    args = parser.parse_args()

    if args.log is not None:
	    log_lvl_number = int(getattr(logging, args.log.upper(), None))
    else:
	    log_lvl_number = logging.DEBUG

    profiler = RttNordicProfilerHost(event_filename=args.event_csv,
                                     event_types_filename=args.event_descr,
                                     log_lvl=log_lvl_number)
    profiler.get_events_descriptions()
    profiler.read_events_rtt(args.time)
    profiler.disconnect()

if __name__ == "__main__":
    main()


