#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from plot_nordic import PlotNordic
from rtt_nordic_profiler_host import RttNordicProfilerHost

import argparse
import threading
import queue
import sys
import logging

def rtt_thread(queue, finish_event, event_filename, event_types_filename, log_lvl_number):
    profiler = RttNordicProfilerHost(finish_event=finish_event, queue=queue,
                                     event_filename=event_filename,
                                     event_types_filename=event_types_filename,
                                     log_lvl=log_lvl_number)
    profiler.get_events_descriptions()
    profiler.read_events_rtt(-1)

def main():
    parser = argparse.ArgumentParser(
        description='Collecting data from Nordic profiler for given time and saving to files.')
    parser.add_argument('dataset_name', help='Name of dataset')
    parser.add_argument('--log', help='Log level')
    args = parser.parse_args()

    if args.log is not None:
        log_lvl_number = int(getattr(logging, args.log.upper(), None))
    else:
        log_lvl_number=logging.WARNING

    ev = threading.Event()
    que = queue.Queue()
    t_rtt = threading.Thread(
        target=rtt_thread,
        args=[que, ev, args.dataset_name + ".csv",
              args.dataset_name + ".json", log_lvl_number])
    t_rtt.start()

    pn = PlotNordic(log_lvl=log_lvl_number)
    pn.plot_events_real_time(que, ev, None, one_line=False)

if __name__ == "__main__":
    main()
