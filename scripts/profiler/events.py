#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import csv
from io import StringIO
from ast import literal_eval



class Event():
    def __init__(self, type_id, timestamp, data):
        self.type_id = type_id
        self.timestamp = timestamp
        self.data = data

    def __str__(self):
        return "ID: {} Timestamp: {} Data: {}".format(self.type_id, self.timestamp, self.data)


class EventType():
    def __init__(self, name, data_types, data_descriptions):
        self.name = name
        self.data_types = data_types
        self.data_descriptions = data_descriptions

    def __str__(self):
        return "Name: {} Data: {}".format(self.name, ",".join([" ".join([i, j]) for i,j in zip(self.data_types, self.data_descriptions)]))

    def serialize(self):
        return {"name": self.name, "data_types": self.data_types,
                "data_descriptions": self.data_descriptions}

    @staticmethod
    def deserialize(json):
        return EventType(
            json["name"],
            json["data_types"],
            json["data_descriptions"])


class TrackedEvent():
    TRACKED_EVENT_FIELDNAMES = ['type_id', 'timestamp', 'data', 'proc_start_time', 'proc_end_time']

    def __init__(self, submit, start_time, end_time):
        self.submit = submit
        self.proc_start_time = start_time
        self.proc_end_time = end_time

    def serialize(self):
        si = StringIO()
        wr = csv.DictWriter(si, delimiter=',', fieldnames=TrackedEvent.TRACKED_EVENT_FIELDNAMES)
        wr.writerow({
                     'type_id': self.submit.type_id,
                     'timestamp': self.submit.timestamp,
                     'data': self.submit.data,
                     'proc_start_time': self.proc_start_time,
                     'proc_end_time': self.proc_end_time
                    })
        return si.getvalue().strip('\r\n')

    @staticmethod
    def deserialize(scsv):
        f_row = StringIO(scsv)
        reader = csv.DictReader(f_row, fieldnames=TrackedEvent.TRACKED_EVENT_FIELDNAMES, delimiter=',')
        rows = list(reader)
        assert len(rows) == 1
        row = rows[0]
        type_id = int(row['type_id'])
        timestamp = float(row['timestamp'])
        data_string = row['data']
        data = literal_eval(data_string)
        proc_start_time = float(row['proc_start_time']) if row['proc_start_time'] != '' else None
        proc_end_time = float(row['proc_end_time']) if row['proc_end_time'] != '' else None
        return TrackedEvent(Event(type_id, timestamp, data), proc_start_time, proc_end_time)

class EventsData():
    def __init__(self, events, registered_events_types):
        self.events = events
        self.registered_events_types = registered_events_types

    def get_event_type_id(self, type_name):
        for key, value in self.registered_events_types.items():
            if type_name == value.name:
                return key
        return None
