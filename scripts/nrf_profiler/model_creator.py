#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import sys
from enum import Enum
import logging
import json
from rtt_nordic_config import RttNordicConfig
from events import Event, EventType, TrackedEvent, EventsData
from processed_events import ProcessedEvents
from stream import StreamError
from io import StringIO
import csv

class Command(Enum):
    START = 1
    STOP = 2
    INFO = 3

NRF_PROFILER_FATAL_ERROR_EVENT_NAME = "_nrf_profiler_fatal_error_event_"

class ModelCreator:

    def __init__(self, stream,
                 event_close,
                 sending_events=False,
                 config=RttNordicConfig,
                 event_filename=None,
                 event_types_filename=None,
                 log_lvl=logging.INFO):

        self.config = config
        self.event_filename = event_filename
        self.event_types_filename = event_types_filename
        self.csvfile = None

        self.event_close = event_close

        timeouts = {
            'descriptions': 1,
            'events': 1
        }
        self.stream = stream
        self.stream.set_timeouts(timeouts)
        self.sending = sending_events

        self.timestamp_overflows = 0
        self.after_half = False

        self.processed_events = ProcessedEvents()
        self.temp_events = []
        self.submitted_event_type = None
        self.raw_data = EventsData([], {})
        self.event_processing_start_id = None
        self.event_processing_end_id = None
        self.submit_event = None
        self.start_event = None

        self.bufs = list()
        self.bcnt = 0

        self.logger = logging.getLogger('Profiler model creator')
        self.logger_console = logging.StreamHandler()
        self.logger.setLevel(log_lvl)
        self.log_format = logging.Formatter('[%(levelname)s] %(name)s: %(message)s')
        self.logger_console.setFormatter(self.log_format)
        self.logger.addHandler(self.logger_console)

    def shutdown(self):
        if self.csvfile is not None:
            self.processed_events.finish_writing_data_to_files(self.csvfile,
                                                               self.event_filename,
                                                               self.event_types_filename)

    def _get_buffered_data(self, num_bytes):
        buf = bytearray()
        while len(buf) < num_bytes:
            tbuf = self.bufs[0]
            size = num_bytes - len(buf)
            if len(tbuf) <= size:
                buf.extend(tbuf)
                del self.bufs[0]
            else:
                buf.extend(tbuf[0:size])
                self.bufs[0] = tbuf[size:]
        self.bcnt -= num_bytes
        return buf

    def _read_bytes(self, num_bytes):
        while True:
            if self.bcnt >= num_bytes:
                break
            try:
                buf = self.stream.recv_ev()
            except StreamError as err:
                if err.args[1] == StreamError.TIMEOUT_MSG:
                    if self.event_close.is_set():
                        self.close()
                    continue
                self.logger.error("Receiving error: {}".format(err))
                self.close()
            if len(buf) > 0:
                self.bufs.append(buf)
                self.bcnt += len(buf)

        return self._get_buffered_data(num_bytes)

    def _timestamp_from_ticks(self, clock_ticks):
        ts_ticks_aggregated = self.timestamp_overflows * self.config['timestamp_raw_max']
        ts_ticks_aggregated += clock_ticks
        ts_s = ts_ticks_aggregated * self.config['ms_per_timestamp_tick'] / 1000
        return ts_s

    def transmit_all_events_descriptions(self):
        while True:
            try:
                bytes = self.stream.recv_desc()
                break
            except StreamError as err:
                if err.args[1] == StreamError.TIMEOUT_MSG:
                    if self.event_close.is_set():
                        self.logger.info("Module closed before receiving event descriptions.")
                        sys.exit()
                    continue
                self.logger.error("Receiving error: {}. Exiting".format(err))
                sys.exit()
        desc_buf = bytes.decode()
        f = StringIO(desc_buf)
        reader = csv.reader(f, delimiter=',')
        for row in reader:
            # Empty field is sent after last event description
            if len(row) == 0:
                break
            name = row[0]
            id = int(row[1])
            data_type = row[2:len(row) // 2 + 1]
            data = row[len(row) // 2 + 1:]
            self.raw_data.registered_events_types[id] = EventType(name, data_type, data)
            if name not in ('event_processing_start', 'event_processing_end'):
                self.processed_events.registered_events_types[id] = EventType(name, data_type, data)

        self.event_processing_start_id = \
            self.raw_data.get_event_type_id('event_processing_start')
        self.event_processing_end_id = \
            self.raw_data.get_event_type_id('event_processing_end')

        if self.sending:
            event_types_dict = dict((k, v.serialize())
                    for k, v in self.processed_events.registered_events_types.items())
            json_et_string = json.dumps(event_types_dict)
            try:
                self.stream.send_desc(json_et_string.encode())
            except StreamError as err:
                self.logger.error("Sending error: {}. Cannot send descriptions.".format(err))
                sys.exit()

    def _read_single_event(self):
        id = int.from_bytes(
            self._read_bytes(1),
            byteorder=self.config['byteorder'],
            signed=False)
        et = self.raw_data.registered_events_types[id]

        buf = self._read_bytes(4)
        timestamp_raw = (
            int.from_bytes(
                buf,
                byteorder=self.config['byteorder'],
                signed=False))

        if self.after_half \
        and timestamp_raw < 0.4 * self.config['timestamp_raw_max']:
            self.timestamp_overflows += 1
            self.after_half = False

        if timestamp_raw > 0.6 * self.config['timestamp_raw_max']:
            self.after_half = True

        timestamp = self._timestamp_from_ticks(timestamp_raw)

        def process_int32(self, data):
            buf = self._read_bytes(4)
            data.append(int.from_bytes(buf, byteorder=self.config['byteorder'],
                                       signed=True))

        def process_uint32(self, data):
            buf = self._read_bytes(4)
            data.append(int.from_bytes(buf, byteorder=self.config['byteorder'],
                                       signed=False))

        def process_int16(self, data):
            buf = self._read_bytes(2)
            data.append(int.from_bytes(buf, byteorder=self.config['byteorder'],
                                       signed=True))

        def process_uint16(self, data):
            buf = self._read_bytes(2)
            data.append(int.from_bytes(buf, byteorder=self.config['byteorder'],
                                       signed=False))

        def process_int8(self, data):
            buf = self._read_bytes(1)
            data.append(int.from_bytes(buf, byteorder=self.config['byteorder'],
                                       signed=True))

        def process_uint8(self, data):
            buf = self._read_bytes(1)
            data.append(int.from_bytes(buf, byteorder=self.config['byteorder'],
                                       signed=False))

        def process_string(self, data):
            buf = self._read_bytes(1)
            buf = self._read_bytes(int.from_bytes(buf, byteorder=self.config['byteorder'],
                                                  signed=False))
            data.append(buf.decode())

        READ_BYTES = {
            "u8": process_uint8,
            "s8": process_int8,
            "u16": process_uint16,
            "s16": process_int16,
            "u32": process_uint32,
            "s32": process_int32,
            "s": process_string,
            "t": process_uint32
        }
        data=[]
        for event_data_type in et.data_types:
            READ_BYTES[event_data_type](self, data)
        return Event(id, timestamp, data)

    def _send_event(self, tracked_event):
        event_string = tracked_event.serialize()
        try:
            self.stream.send_ev(event_string.encode())
        except StreamError as err:
            self.logger.error("Sending error: {}. Cannot send event.".format(err))
            self.close()

    def _write_event_to_file(self, csvfile, tracked_event):
        try:
            csvfile.write(tracked_event.serialize() + '\r\n')
        except IOError:
            self.logger.error("Problem with accessing csv file")
            self.close()

    def transmit_events(self):
        if self.event_filename and self.event_types_filename:
            self.csvfile = self.processed_events.init_writing_data_to_files(
                self.event_filename,
                self.event_types_filename)
        while True:
            event = self._read_single_event()
            if self.raw_data.registered_events_types[event.type_id].name == NRF_PROFILER_FATAL_ERROR_EVENT_NAME:
                self.logger.error("Fatal error of Profiler on device! Event has been dropped. "
                                  "Data buffer has overflown. No more events will be received.")

            if event.type_id == self.event_processing_start_id:
                self.start_event = event
                for i in range(len(self.temp_events) - 1, -1, -1):
                    # comparing memory addresses of event processing start
                    # and event submit to identify matching events
                    if self.temp_events[i].data[0] == self.start_event.data[0]:
                        self.submit_event = self.temp_events[i]
                        self.submitted_event_type = self.submit_event.type_id
                        del self.temp_events[i]
                        break

            elif event.type_id == self.event_processing_end_id:
                # comparing memory addresses of event processing start and
                # end to identify matching events
                if self.submitted_event_type is not None and event.data[0] \
                            == self.start_event.data[0]:
                    tracked_event = TrackedEvent(
                            self.submit_event,
                            self.start_event.timestamp,
                            event.timestamp)
                    if self.csvfile is not None:
                        self._write_event_to_file(self.csvfile, tracked_event)
                    if self.sending:
                        self._send_event(tracked_event)
                    self.submitted_event_type = None

            elif not self.processed_events.is_event_tracked(event.type_id):
                tracked_event = TrackedEvent(event, None, None)
                if self.csvfile is not None:
                    self._write_event_to_file(self.csvfile, tracked_event)
                if self.sending:
                    self._send_event(tracked_event)

            else:
                self.temp_events.append(event)

    def start(self):
        self.transmit_all_events_descriptions()
        self.transmit_events()

    def close(self):
        self.logger.info("Real time transmission closed")
        self.shutdown()
        self.logger.info("Events data saved to files")
        sys.exit()
