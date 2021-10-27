#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import os
from multiprocessing import Process, Event, active_children
import argparse
import signal
import time
import logging
from rtt2socket import Rtt2Socket
from model_creator import ModelCreator

RTT2SOCKET_OUT_ADDR_DICT = {
    'descriptions': '\0' + 'rtt2socket_desc',
    'events': '\0' + 'rtt2socket_ev'
}

MODEL_CREATOR_IN_ADDR_DICT = {
    'descriptions': '\0' + 'model_creator_desc',
    'events': '\0' + 'model_creator_ev'
}

def rtt2socket(event, log_lvl_number):
    try:
        rtt2s = Rtt2Socket(RTT2SOCKET_OUT_ADDR_DICT, MODEL_CREATOR_IN_ADDR_DICT, log_lvl=log_lvl_number)
        event.wait()
        rtt2s.read_and_transmit_data()
    except KeyboardInterrupt:
        rtt2s.close()

def model_creator(event, dataset_name, log_lvl_number):
    try:
        mc = ModelCreator(MODEL_CREATOR_IN_ADDR_DICT,
                          event_filename=dataset_name + ".csv",
                          event_types_filename=dataset_name + ".json",
                          log_lvl=log_lvl_number)
        event.set()
        mc.start()
    except KeyboardInterrupt:
        mc.close()


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
        log_lvl_number = logging.INFO

    # event is made to ensure that ModelCreator class is initialized before Rtt2Socket starts sending data
    event = Event()

    try:
        processes = []
        processes.append(Process(target=rtt2socket,
                                 args=(event, log_lvl_number),
                                 daemon=True))
        processes.append(Process(target=model_creator,
                                 args=(event, args.dataset_name, log_lvl_number),
                                 daemon=True))
        for p in processes:
            p.start()

        start_time = time.time()
        is_waiting = True
        while is_waiting:
            for p in processes:
                p.join(timeout=0.5)
                # Terminate other processes if one of the processes is not active.
                if len(processes) > len(active_children()):
                    is_waiting = False
                    break

                # Terminate every process on timeout.
                if (time.time() - start_time) >= args.time:
                    is_waiting = False
                    break

        for p in processes:
            if p.is_alive():
                os.kill(p.pid, signal.SIGINT)
                # Ensure that we stop processes in order to prevent profiler data drop.
                p.join()

    except KeyboardInterrupt:
        for p in processes:
            p.join()

if __name__ == "__main__":
    main()
