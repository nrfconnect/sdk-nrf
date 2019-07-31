#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

from events import EventsData, Event, EventType
import matplotlib.pyplot as plt
import argparse


def main():
    descr = "Merge data from device and dongle. ble_peer_events should be" \
            " registered at the beginning and at the end of measurements"  \
            " (used to compensate clock drift)."
    parser = argparse.ArgumentParser(description=descr)
    parser.add_argument("device_dataset", help="Name of device dataset")
    parser.add_argument("dongle_dataset", help="Name of dongle dataset")
    parser.add_argument('result_dataset', help="Name for result dataset")
    args = parser.parse_args()

    evt_device = EventsData([], {})
    evt_device.read_data_from_files(args.device_dataset + ".csv",
                                    args.device_dataset + ".json")

    evt_dongle = EventsData([], {})
    evt_dongle.read_data_from_files(args.dongle_dataset + ".csv",
                                    args.dongle_dataset + ".json")

    # Compensating clock drift - based on ble_peer_event
    peer_evt_device = evt_device.get_event_type_id("ble_peer_event")
    peer_evt_dongle = evt_dongle.get_event_type_id("ble_peer_event")

    peer_device = list(filter(lambda x: x.type_id == peer_evt_device,
                             evt_device.events))
    peer_dongle = list(filter(lambda x: x.type_id == peer_evt_dongle,
                              evt_dongle.events))


    diff_start = peer_device[0].timestamp - peer_dongle[0].timestamp
    diff_end = peer_device[-1].timestamp - peer_dongle[-1].timestamp
    diff_time = peer_dongle[-1].timestamp - peer_dongle[0].timestamp
    time_start = peer_dongle[0].timestamp

    # Using linear approximation of clock drift between device and dongle
    # t_dongle = t_device + (diff_time * a) + b

    b = diff_start
    a = (diff_end - diff_start) / diff_time

    B_DIFF_THRESH = 0.1
    if abs(diff_end - diff_start) > B_DIFF_THRESH:
        print("Clock drift difference between beginnning and end is high.")
        print("This could be caused by measurements missmatch or very long" \
              " measurement time.")

    # Reindexing, renaming and compensating time differences for dongle events
    max_device_id = max([int(i) for i in evt_device.registered_events_types])

    evt_dongle.events = list(map(lambda x: Event(x.type_id + max_device_id + 1,
                            x.timestamp + a * (x.timestamp - time_start) + b,
                            x.data),
                            evt_dongle.events))

    evt_dongle.registered_events_types = {k + max_device_id + 1 :
                EventType(v.name + "_dongle", v.data_types, v.data_descriptions)
                for k, v in evt_dongle.registered_events_types.items()}

    evt_device.registered_events_types = {k :
                EventType(v.name + "_device", v.data_types, v.data_descriptions)
                for k, v in evt_device.registered_events_types.items()}

    all_registered_events_types = evt_device.registered_events_types.copy()
    all_registered_events_types.update(evt_dongle.registered_events_types)

    result_events = EventsData(evt_device.events + evt_dongle.events,
                               all_registered_events_types)

    result_events.write_data_to_files(args.result_dataset + ".csv",
                                      args.result_dataset + ".json")


if __name__ == "__main__":
    main()
