#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

from pynrfjprog import API
import time
import matplotlib.pyplot as plt
from datetime import datetime
import queue
import sys
import threading
from enum import Enum
from rtt_nordic_config import RttNordicConfig
from events import Event, EventType, EventsData
import logging

class Command(Enum):
    START = 1
    STOP = 2
    INFO = 3


class RttNordicProfilerHost:

    def __init__(self, config=RttNordicConfig, finish_event=None,
                 queue=None, event_filename=None,
                 event_types_filename=None, log_lvl=logging.WARNING):
        self.event_filename = event_filename
        self.event_types_filename = event_types_filename
        self.config = config
        self.finish_event = finish_event
        self.queue = queue
        self.received_events = EventsData([], {})
        self.timestamp_overflows = 0
        self.after_half = False

        self.desc_buf = ""
        self.bufs = list()
        self.bcnt = 0
        self.last_read_time = time.time()
        self.reading_data = True

        self.logger = logging.getLogger('RTT Profiler Host')
        self.logger_console = logging.StreamHandler()
        self.logger.setLevel(log_lvl)
        self.log_format = logging.Formatter('[%(levelname)s] %(name)s: %(message)s')
        self.logger_console.setFormatter(self.log_format)
        self.logger.addHandler(self.logger_console)

        self.connect()

    def rtt_get_device_family():
        with API.API(API.DeviceFamily.UNKNOWN) as api:
            api.connect_to_emu_without_snr()
            return api.read_device_family()

    def connect(self):
        try:
            self.jlink = API.API(self.config['device_family'])
        except ValueError:
            self.logger.warning('Unrecognized device family. Trying to recognize automatically')
            self.config['device_family'] = rtt_nordic_profiler_host.rtt_get_device_family()
            self.logger.info('Recognized device family: ' + self.config['device_family'])
            self.jlink = API.API(self.config['device_family'])

        self.jlink.open()
        if self.config['device_snr'] is not None:
            self.jlink.connect_to_emu_with_snr(self.config['device_snr'])
        else:
            self.jlink.connect_to_emu_without_snr()

        if self.config['reset_on_start']:
            self.jlink.sys_reset()
            self.jlink.go()
        self.jlink.rtt_start()

        TIMEOUT = 20
        start_time = time.time()
        while not self.jlink.rtt_is_control_block_found():
            if time.time() - start_time > TIMEOUT:
                self.logger.error("Cannot find RTT control block")
                sys.exit()

            time.sleep(1)

        self.logger.info("Connected to device via RTT")

    def shutdown(self):
        self.disconnect()
        self._read_remaining_events()
        if self.event_filename and self.event_types_filename:
            self.received_events.write_data_to_files(self.event_filename,
                                                     self.event_types_filename)

    def disconnect(self):
        self.stop_logging_events()
        # read remaining data to buffer
        try:
            buf = self.jlink.rtt_read(self.config['rtt_data_channel'],
                                      self.config['rtt_read_chunk_size'],
                                      encoding=None)

        except:
            self.logger.error("Problem with reading RTT data.")

        while len(buf) > 0:
            self.bufs.append(buf)
            self.bcnt += len(buf)
            buf = self.jlink.rtt_read(self.config['rtt_data_channel'],
                                      self.config['rtt_read_chunk_size'],
                                      encoding=None)
        try:
            self.jlink.rtt_stop()
            self.jlink.disconnect_from_emu()
            self.jlink.close()

        except:
            self.logger.error("JLink connection lost. Saving collected data.")
            return

        self.logger.info("Disconnected from device")

    def _get_buffered_data(self, num_bytes):
        buf = bytearray()
        while len(buf) < num_bytes:
            tbuf = self.bufs[0]
            size = num_bytes - len(buf)
            if len(tbuf) <= size:
                buf = buf + tbuf
                del(self.bufs[0])
            else:
                buf = buf + tbuf[0:size]
                self.bufs[0] = tbuf[size:]
        self.bcnt -= num_bytes
        return buf

    def _read_bytes(self, num_bytes):
        now = time.time()

        while self.reading_data:
            if now - self.last_read_time < self.config['rtt_read_period'] \
            and self.bcnt >= num_bytes:
                break

            try:
                buf = self.jlink.rtt_read(self.config['rtt_data_channel'],
                                          self.config['rtt_read_chunk_size'],
                                          encoding=None)
            except:
                self.logger.error("Problem with reading RTT data.")
                self.shutdown()

            if len(buf) > 0:
                self.bufs.append(buf)
                self.bcnt += len(buf)

            if len(buf) > self.config['rtt_additional_read_thresh']:
                continue

            self.last_read_time = now

            if self.bcnt >= num_bytes:
                break

            if self.finish_event is not None and self.finish_event.is_set():
                self.finish_event.clear()
                self.logger.info("Real time transmission closed")
                self.shutdown()
                self.logger.info("Events data saved to files")
                sys.exit()

            time.sleep(0.05)

        return self._get_buffered_data(num_bytes)

    def _calculate_timestamp_from_clock_ticks(self, clock_ticks):
        return self.config['ms_per_timestamp_tick'] * (
            clock_ticks + self.timestamp_overflows * self.config['timestamp_raw_max']) / 1000

    def _read_single_event_description(self):
        while '\n' not in self.desc_buf:
            try:
                buf_temp = self.jlink.rtt_read(self.config['rtt_info_channel'],
                                      self.config['rtt_read_chunk_size'],
                                      encoding='utf-8')

            except:
                self.logger.error("Problem with reading RTT data.")
                self.shutdown()

            self.desc_buf += buf_temp
            time.sleep(0.1)

        desc = str(self.desc_buf[0:self.desc_buf.find('\n')])
        # Empty field is send after last event description
        if len(desc) == 0:
            return None, None
        self.desc_buf = self.desc_buf[self.desc_buf.find('\n')+1:]

        desc_fields = desc.split(',')

        name = desc_fields[0]
        id = int(desc_fields[1])
        data_type = []
        for i in range(2, len(desc_fields) // 2 + 1):
            data_type.append(desc_fields[i])
        data = []
        for i in range(len(desc_fields) // 2 + 1, len(desc_fields)):
            data.append(desc_fields[i])
        return id, EventType(name, data_type, data)

    def _read_all_events_descriptions(self):
        while True:
            id, et = self._read_single_event_description()
            if (id is None or et is None):
                break
            self.received_events.registered_events_types[id] = et

    def get_events_descriptions(self):
        self._send_command(Command.INFO)
        self._read_all_events_descriptions()
        if self.queue is not None:
            self.queue.put(self.received_events.registered_events_types)
        self.logger.info("Received events descriptions")
        self.logger.info("Ready to start logging events")

    def _read_single_event_rtt(self):
        id = int.from_bytes(
            self._read_bytes(1),
            byteorder=self.config['byteorder'],
            signed=False)
        et = self.received_events.registered_events_types[id]

        buf = self._read_bytes(4)
        timestamp_raw = (
            int.from_bytes(
                buf,
                byteorder=self.config['byteorder'],
                signed=False))

        if self.after_half \
        and timestamp_raw < 0.2 * self.config['timestamp_raw_max']:
            self.timestamp_overflows += 1
            self.after_half = False

        if timestamp_raw > 0.6 * self.config['timestamp_raw_max'] \
        and timestamp_raw < 0.9 * self.config['timestamp_raw_max']:
            self.after_half = True

        timestamp = self._calculate_timestamp_from_clock_ticks(timestamp_raw)

        data = []
        for i in et.data_types:
            signum = False
            if i[0] == 's':
                signum = True
            buf = self._read_bytes(4)
            data.append(int.from_bytes(buf, byteorder=self.config['byteorder'],
                                       signed=signum))
        return Event(id, timestamp, data)

    def _read_remaining_events(self):
        self.reading_data = False
        while self.bcnt != 0:
            event = self._read_single_event_rtt()
            self.received_events.events.append(event)
            if self.queue is not None:
                self.queue.put(event)

        # End of transmission
        if self.queue is not None:
            self.queue.put(None)

    def read_events_rtt(self, time_seconds):
        self.logger.info("Start logging events data")
        self.start_logging_events()
        start_time = time.time()
        current_time = start_time
        while current_time - start_time < time_seconds or time_seconds < 0:
            event = self._read_single_event_rtt()
            self.received_events.events.append(event)
            if self.queue is not None:
                self.queue.put(event)
            current_time = time.time()
        self.logger.info("Real time transmission closed")
        self.shutdown()
        self.logger.info("Events data saved to files")
        sys.exit()

    def start_logging_events(self):
        self._send_command(Command.START)

    def stop_logging_events(self):
        self._send_command(Command.STOP)

    def _send_command(self, command_type):
        command = bytearray(1)
        command[0] = command_type.value
        try:
            self.jlink.rtt_write(self.config['rtt_command_channel'], command, None)
        except:
            self.logger.error("Problem with writing RTT data.")
