#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

from events import EventsData, Event, EventType
import matplotlib.pyplot as plt
import argparse


def main():
    descr = "Merge data from Peripheral and Central. Synchronization events" \
            " should be registered at the beginning and at the end of" \
            " measurements (used to compensate clock drift)."
    parser = argparse.ArgumentParser(description=descr)
    parser.add_argument("peripheral_dataset", help="Name of Peripheral dataset")
    parser.add_argument("peripheral_sync_event",
                        help="Event used for synchronization - Peripheral")
    parser.add_argument("central_dataset", help="Name of Central dataset")
    parser.add_argument("central_sync_event",
                        help="Event used for synchronization - Central")
    parser.add_argument("result_dataset", help="Name for result dataset")
    args = parser.parse_args()

    evt_peripheral = EventsData([], {})
    evt_peripheral.read_data_from_files(args.peripheral_dataset + ".csv",
                                        args.peripheral_dataset + ".json")

    evt_central = EventsData([], {})
    evt_central.read_data_from_files(args.central_dataset + ".csv",
                                     args.central_dataset + ".json")

    # Compensating clock drift - based on synchronization events
    sync_evt_peripheral = evt_peripheral.get_event_type_id(
            args.peripheral_sync_event)
    sync_evt_central = evt_central.get_event_type_id(args.central_sync_event)

    sync_peripheral = list(filter(lambda x: x.type_id == sync_evt_peripheral,
                             evt_peripheral.events))
    sync_central = list(filter(lambda x: x.type_id == sync_evt_central,
                              evt_central.events))

    if len(sync_central) < 2 or len(sync_peripheral) < 2:
        print("Not enough synchronization events (require at least two)")
        return

    diff_start = sync_peripheral[0].timestamp - sync_central[0].timestamp
    diff_end = sync_peripheral[-1].timestamp - sync_central[-1].timestamp
    diff_time = sync_central[-1].timestamp - sync_central[0].timestamp
    time_start = sync_central[0].timestamp

    # Using linear approximation of clock drift between peripheral and central
    # t_central = t_peripheral + (diff_time * a) + b

    b = diff_start
    a = (diff_end - diff_start) / diff_time

    B_DIFF_THRESH = 0.1
    if abs(diff_end - diff_start) > B_DIFF_THRESH:
        print("Clock drift difference between beginnning and end is high.")
        print("This could be caused by measurements missmatch or very long" \
              " measurement time.")

    # Reindexing, renaming and compensating time differences for central events
    max_peripheral_id = max([int(i)
                            for i in evt_peripheral.registered_events_types])

    evt_central.events = list(map(lambda x:
                            Event(x.type_id + max_peripheral_id + 1,
                            x.timestamp + a * (x.timestamp - time_start) + b,
                            x.data),
                            evt_central.events))

    evt_central.registered_events_types = {k + max_peripheral_id + 1 :
                EventType(v.name + "_central", v.data_types, v.data_descriptions)
                for k, v in evt_central.registered_events_types.items()}

    evt_peripheral.registered_events_types = {
                k : EventType(v.name + "_peripheral",
                              v.data_types, v.data_descriptions)
                for k, v in evt_peripheral.registered_events_types.items()}

    all_registered_events_types = evt_peripheral.registered_events_types.copy()
    all_registered_events_types.update(evt_central.registered_events_types)

    result_events = EventsData(evt_peripheral.events + evt_central.events,
                               all_registered_events_types)

    result_events.write_data_to_files(args.result_dataset + ".csv",
                                      args.result_dataset + ".json")


if __name__ == "__main__":
    main()
