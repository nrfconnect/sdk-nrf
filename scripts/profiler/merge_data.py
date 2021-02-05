#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from events import EventsData, Event, EventType
import argparse
import numpy as np


def sync_peripheral_ts(ts_peripheral, sync_ts_peripheral, sync_ts_central):
    MSE_THRESH = 1e-9
    # Predict using linear regression only if the Mean Squared Error is low.
    # In general, using Crystal Oscillator results in lower MSE and using
    # RC oscillator results in higher MSE.
    model = np.polyfit(sync_ts_peripheral, sync_ts_central, 1)
    predictor = np.poly1d(model)

    new_sync_ts_peripheral = predictor(sync_ts_peripheral)
    mse = np.square(np.subtract(sync_ts_central, new_sync_ts_peripheral)).mean()

    if mse < MSE_THRESH:
        print('Use linear regression')
        return predictor(ts_peripheral)
    else:
        print('Use piecewise linear interpolation')
        return np.interp(ts_peripheral, sync_ts_peripheral, sync_ts_central,
                         left=-1, right=-1)


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

    ts_peripheral = list(map(lambda x: x.timestamp, evt_peripheral.events))
    sync_ts_peripheral = list(map(lambda x: x.timestamp, sync_peripheral))
    sync_ts_central = list(map(lambda x: x.timestamp, sync_central))

    sync_diffs_central = np.subtract(sync_ts_central[1:], sync_ts_central[:-1])
    sync_diffs_peripheral = np.subtract(sync_ts_peripheral[1:], sync_ts_peripheral[:-1])

    rounded_diffs_central = list(map(lambda x: round(x, 1), sync_diffs_central))
    rounded_diffs_peripheral = list(map(lambda x: round(x, 1), sync_diffs_peripheral))

    shift_c = rounded_diffs_central.index(rounded_diffs_peripheral[0])
    shift_p = rounded_diffs_peripheral.index(rounded_diffs_central[0])

    if shift_c < shift_p:
        sync_ts_central = sync_ts_central[shift_c:]
    elif shift_p < shift_c:
        sync_ts_peripheral = sync_ts_peripheral[shift_p:]

    if len(sync_ts_central) < len(sync_ts_peripheral):
        sync_ts_peripheral = sync_ts_peripheral[:len(sync_ts_central)]
    elif len(sync_ts_peripheral) < len(sync_ts_central):
        sync_ts_central = sync_ts_central[:len(sync_ts_peripheral)]

    new_ts_peripheral = sync_peripheral_ts(ts_peripheral,
                                           sync_ts_peripheral,
                                           sync_ts_central)
    assert len(new_ts_peripheral) == len(ts_peripheral)

    # Reindexing, renaming and compensating time differences for peripheral events
    max_central_id = max([int(i) for i in evt_central.registered_events_types])

    assert len(new_ts_peripheral) == len(evt_peripheral.events)
    evt_peripheral.events = list(map(lambda x, y:
                                 Event(x.type_id + max_central_id + 1,
                                       y,
                                       x.data),
                                 evt_peripheral.events, new_ts_peripheral))

    evt_peripheral.registered_events_types = {k + max_central_id + 1 :
                EventType(v.name + "_peripheral", v.data_types, v.data_descriptions)
                for k, v in evt_peripheral.registered_events_types.items()}

    evt_central.registered_events_types = {
                k : EventType(v.name + "_central",
                              v.data_types, v.data_descriptions)
                for k, v in evt_central.registered_events_types.items()}

    # Filter out events that are out of synchronization period
    TIME_DIFF = 0.5
    start_time = sync_ts_central[0] - TIME_DIFF
    end_time = sync_ts_central[-1] + TIME_DIFF
    evt_peripheral.events = list(filter(lambda x:
        x.timestamp >= start_time and x.timestamp <= end_time,
        evt_peripheral.events))

    evt_central.events = list(filter(lambda x:
        x.timestamp >= start_time and x.timestamp <= end_time,
        evt_central.events))

    all_registered_events_types = evt_peripheral.registered_events_types.copy()
    all_registered_events_types.update(evt_central.registered_events_types)

    result_events = EventsData(evt_peripheral.events + evt_central.events,
                               all_registered_events_types)

    result_events.write_data_to_files(args.result_dataset + ".csv",
                                      args.result_dataset + ".json")

    print('Profiler data merged successfully')

if __name__ == "__main__":
    main()
