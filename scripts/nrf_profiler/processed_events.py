#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from events import TrackedEvent, EventType
import logging
import csv
import json
import hashlib
import sys

EM_MEM_ADDRESS_DATA_DESC = "_em_mem_address_"

class ProcessedEvents():
    def __init__(self):
        self.registered_events_types = {}
        self.tracked_events = []

        self.logger = logging.getLogger('Processed Events')
        self.logger_console = logging.StreamHandler()
        self.logger.setLevel(logging.WARNING)
        self.log_format = logging.Formatter(
            '[%(levelname)s] %(name)s: %(message)s')
        self.logger_console.setFormatter(self.log_format)
        self.logger.addHandler(self.logger_console)

    def get_event_type_id(self, type_name):
        for key, value in self.registered_events_types.items():
            if type_name == value.name:
                return key
        return None

    def is_event_tracked(self, event_type_id):
        event_data_descriptions = self.registered_events_types[event_type_id].data_descriptions
        if len(event_data_descriptions) == 0 or event_data_descriptions[0] != EM_MEM_ADDRESS_DATA_DESC:
            return False
        return True

    def verify(self):
        for ev in self.tracked_events:
            if ev.submit.type_id not in self.registered_events_types:
                return False
        return True

    def init_writing_data_to_files(self, filename_events, filename_event_types):
        csvfile = self._init_writing_events_csv(filename_events)
        self._write_events_types_json_without_hash(filename_event_types)
        return csvfile

    def finish_writing_data_to_files(self, csvfile, filename_events, filename_event_types):
        csvfile.close()
        csv_hash = ProcessedEvents._calculate_md5_hash_of_file(filename_events)
        self._write_events_types_json(filename_event_types, csv_hash)

    def write_data_to_files(self, filename_events, filename_event_types):
        self._write_events_csv(filename_events)
        csv_hash = ProcessedEvents._calculate_md5_hash_of_file(filename_events)
        self._write_events_types_json(filename_event_types, csv_hash)

    def read_data_from_files(self, filename_events, filename_event_types):
        csv_hash1 = self._read_events_types_json(filename_event_types)
        self._read_events_csv(filename_events)
        csv_hash2 = ProcessedEvents._calculate_md5_hash_of_file(filename_events)
        if csv_hash1 != csv_hash2:
            self.logger.warning("Hash values of csv files do not match")
            self.logger.warning("Events and descriptions may be inconsistent")
    @staticmethod
    def _calculate_md5_hash_of_file(filename):
        return hashlib.md5(open(filename, 'rb').read()).hexdigest()

    def _init_writing_events_csv(self, filename):
        try:
            csvfile = open(filename, 'w', newline='')
            wr = csv.DictWriter(csvfile, delimiter=',',
                                fieldnames=TrackedEvent.TRACKED_EVENT_FIELDNAMES)
            wr.writeheader()
            return csvfile
        except IOError:
            self.logger.error("Problem with accessing file: " + filename)
            sys.exit()

    def _write_events_csv(self, filename):
        try:
            csvfile = self._init_writing_events_csv(filename)
            for ev in self.tracked_events:
                csvfile.write(ev.serialize() + '\r\n')
            csvfile.close()
        except IOError:
            self.logger.error("Problem with accessing file: " + filename)
            sys.exit()

    def _read_events_csv(self, filename):
        try:
            with open(filename, 'r', newline='') as csvfile:
                header = csvfile.readline()
                header = header.rstrip('\r\n')
                fieldnames = header.split(',')
                assert fieldnames == TrackedEvent.TRACKED_EVENT_FIELDNAMES
                for line in csvfile:
                    self.tracked_events.append(TrackedEvent.deserialize(line))
        except IOError:
            self.logger.error("Problem with accessing file: " + filename)
            sys.exit()

    def _write_events_types_json_without_hash(self, filename):
        d = dict((k, v.serialize())
                 for k, v in self.registered_events_types.items())
        try:
            with open(filename, "w") as wr:
                json.dump(d, wr, indent=4)
        except IOError:
            self.logger.error("Problem with accessing file: " + filename)
            sys.exit()

    def _write_events_types_json(self, filename, csv_hash):
        d = dict((k, v.serialize())
                 for k, v in self.registered_events_types.items())
        d['csv_hash'] = csv_hash
        try:
            with open(filename, "w") as wr:
                json.dump(d, wr, indent=4)
        except IOError:
            self.logger.error("Problem with accessing file: " + filename)
            sys.exit()

    def _read_events_types_json(self, filename):
        try:
            with open(filename, "r") as rd:
                data = json.load(rd)
        except IOError:
            self.logger.error("Problem with accessing file: " + filename)
            sys.exit()
        csv_hash = data['csv_hash']
        del data['csv_hash']
        self.registered_events_types = dict((int(k), EventType.deserialize(v))
                                                     for k, v in data.items())
        return csv_hash
