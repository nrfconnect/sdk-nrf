#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from rtt_nordic_profiler_host import RttNordicProfilerHost
import sys
import argparse
import logging
import signal
import threading


def main():
    parser = argparse.ArgumentParser(
        description='Collecting data from Nordic profiler for given time and saving to files.')
    parser.add_argument('time', type=int, help='Time of collecting data [s]')
    parser.add_argument('dataset_name', help='Name of dataset')
    parser.add_argument('--log', help='Log level')
    args = parser.parse_args()

    if args.log is not None:
	    log_lvl_number = int(getattr(logging, args.log.upper(), None))
    else:
	    log_lvl_number = logging.DEBUG

    def sigint_handler(sig, frame):
        end_ev.set()

    signal.signal(signal.SIGINT, sigint_handler)
    end_ev = threading.Event()

    profiler = RttNordicProfilerHost(
                event_filename=args.dataset_name + ".csv",
                finish_event=end_ev,
                event_types_filename=args.dataset_name + ".json",
                log_lvl=log_lvl_number)
    profiler.get_events_descriptions()
    profiler.read_events_rtt(args.time)

if __name__ == "__main__":
    main()
