#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import argparse
import logging
import signal
import time
from multiprocessing import Event, Process, active_children

from model_creator import ModelCreator
from rtt2stream import Rtt2Stream
from stream import Stream

# timeout for processes synchronization
SYNCHRONIZATION_TIMEOUT = 120
SHUTDOWN_TIMEOUT = 2

is_waiting = True
def signal_handler(sig, frame):
    global is_waiting
    is_waiting = False

def rtt2stream(
        stream,
        event_model_creator, event_close,
        log_lvl_number,
        own_event, prev_event, last_event, time_event,
        process_index
    ):
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    try:
        rtt2s = Rtt2Stream(
            stream,
            event_close,
            own_event, prev_event, last_event,
            process_index,
            log_lvl = log_lvl_number
        )
        # synchronization with own ModelCreator
        event_model_creator.wait()
        # synchronization with main loop
        time_event.set()
        rtt2s.read_and_transmit_data()
    except Exception as e:
        print(f"[ERROR] Unhandled exception in Profiler Rtt to stream module: {e}")

def model_creator(
        stream,
        event, event_close,
        dataset_name,
        log_lvl_number
    ):
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    try:
        mc = ModelCreator(
            stream,
            event_close, sending_events = False,
            event_filename = dataset_name + ".csv",
            event_types_filename = dataset_name + ".json",
            log_lvl = log_lvl_number
        )
        event.set()
        mc.start()
    except Exception as e:
        print(f"[ERROR] Unhandled exception in Profiler model creator module: {e}")


def main():
    signal.signal(signal.SIGINT, signal_handler)

    parser = argparse.ArgumentParser(
        description='Collecting data from Nordic nrf_profiler for given time and saving to files.',
        allow_abbrev=False)
    parser.add_argument('time', type=int, help='Time of collecting data [s]')
    parser.add_argument('dataset_name', help='Name of dataset')
    parser.add_argument('--sync', type=int, choices=range(1, 11), default=1,
                        help='Enable synchronization mode with a specific number of connections (1 to 10).')
    parser.add_argument('--log', help='Log level')
    args = parser.parse_args()

    if args.log is not None:
        log_lvl_number = int(getattr(logging, args.log.upper(), None))
    else:
        log_lvl_number = logging.INFO

    dev_amount = args.sync

    sync_events = [Event() for _ in range(dev_amount)]
    time_sync = [Event() for _ in range(dev_amount)]

    processes = []
    for item in range(0, dev_amount):
        event_model_creator = Event()
        # Setting these events results in closing corresponding modules.
        event_close_rtt2stream = Event()
        event_close_model_creator = Event()
        streams = Stream.create_stream(2)
        dataset_name = args.dataset_name
        if dev_amount > 1:
            dataset_name += '_' + str(item)
        own_event = sync_events[item]
        prev_event = sync_events[item - 1] if item > 0 else None
        last_event = sync_events[dev_amount - 1] if item < dev_amount - 1 else None
        time_event = time_sync[item]

        processes.append(
            (
                Process(
                    target = rtt2stream,
                    args = (
                        streams[0],
                        event_model_creator, event_close_rtt2stream,
                        log_lvl_number,
                        own_event, prev_event, last_event, time_event,
                        item
                    ),
                    daemon = True
                ),
                event_close_rtt2stream
            )
        )

        processes.append(
            (
                Process(
                    target = model_creator,
                    args = (
                        streams[1],
                        event_model_creator, event_close_model_creator,
                        dataset_name,
                        log_lvl_number
                    ),
                    daemon = True
                ),
                event_close_model_creator
            )
        )

    for p, _ in processes:
        p.start()

    print("START measurements.")

    global is_waiting
    start_time = time.time()
    synchronization_done = False
    while is_waiting and ((time.time() - start_time) < SYNCHRONIZATION_TIMEOUT):
        if all(p.is_alive() for p, _ in processes):
            if not all(i.is_set() for i in time_sync):
                time.sleep(0.1)
            else:
                synchronization_done = True
                break
        else:
            is_waiting = False
            break

    if (not synchronization_done) and is_waiting:
        print("[ERROR] Synchronization timeout")
    else:
        start_time = time.time()
        while is_waiting:
            for p, _ in processes:
                p.join(timeout = 0.5)
                # Terminate other processes if one of the processes is not active.
                if len(processes) > len(active_children()):
                    is_waiting = False
                    break

                # Terminate every process on timeout.
                if (time.time() - start_time) >= args.time:
                    is_waiting = False
                    break

    print("STOP measurements.")

    for p, event_close in processes:
        if p.is_alive():
            event_close.set()
            # Ensure that we stop processes in order to prevent nrf_profiler data drop.
            p.join(timeout = SHUTDOWN_TIMEOUT)
        if p.is_alive():
            p.terminate()
            p.join(timeout = SHUTDOWN_TIMEOUT)
        if p.is_alive():
            p.kill()
            p.join(timeout = SHUTDOWN_TIMEOUT)

if __name__ == "__main__":
    main()
