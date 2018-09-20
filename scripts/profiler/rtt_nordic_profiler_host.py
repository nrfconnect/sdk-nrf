# Copyright (c) 2018 Nordic Semiconductor ASA
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
        self.logger = logging.getLogger('RTT Profiler Host')
        self.logger_console = logging.StreamHandler()
        self.logger.setLevel(log_lvl)
        self.log_format = logging.Formatter('[%(levelname)s] %(name)s: %(message)s')
        self.logger_console.setFormatter(self.log_format)
        self.logger.addHandler(self.logger_console)
        self._connect()

    def rtt_get_device_family():
        with API.API(API.DeviceFamily.UNKNOWN) as api:
            api.connect_to_emu_without_snr()
            return api.read_device_family()

    def _connect(self):
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
        time.sleep(1)  # time required for initialization
        self.logger.info("Connected to device via RTT")

    def disconnect(self):
        if self.queue:
            self.queue.put(None)
        self.jlink.rtt_stop()
        self.jlink.disconnect_from_emu()
        self.jlink.close()
        if self.event_filename is not None and self.event_types_filename is not None:
            self.received_events.write_data_to_files(self.event_filename,
                                                     self.event_types_filename)
        self.logger.info("Disconnected from device")

    def _read_char(self, channel):
        if self.config['connection_timeout'] > 0:
            start_time = time.time()
        buf = self.jlink.rtt_read(channel, 1, encoding='utf-8')
        while (len(buf) == 0):
            time.sleep(0.0001)
            buf = self.jlink.rtt_read(channel, 1, encoding='utf-8')
            if self.config['connection_timeout'] > 0 and time.time(
            ) - start_time > self.config['connection_timeout']:
                self.disconnect()
                self.logger.error("Connection timeout")
                sys.exit()
        return buf

    def _read_bytes(self, channel, num_bytes):
        if self.config['connection_timeout'] > 0:
            start_time = time.time()
        buf = self.jlink.rtt_read(channel, num_bytes, encoding=None)
        while (len(buf) < num_bytes):
            time.sleep(0.05)
            buf.extend(
                self.jlink.rtt_read(
                    channel,
                    num_bytes - len(buf),
                    encoding=None))

            if self.config['connection_timeout'] > 0 and time.time(
            ) - start_time > self.config['connection_timeout']:
                self.disconnect()
                self.logger.error("Connection timeout")
                sys.exit()
            if self.finish_event is not None and self.finish_event.is_set():
                self.logger.info("Real time transmission closed")
                self.disconnect()
                sys.exit()
        return buf

    def _calculate_timestamp_from_clock_ticks(self, clock_ticks):
        return self.config['ms_per_timestamp_tick'] * (
            clock_ticks + self.timestamp_overflows * self.config['timestamp_raw_max']) / 1000

    def _read_single_event_description(self):
        buf = self._read_char(self.config['rtt_info_channel'])
        if ('\n' == buf):
            return None, None

        raw_desc = []
        raw_desc.append(buf)
        while buf != '\n':
            buf = self._read_char(self.config['rtt_info_channel'])
            raw_desc.append(buf)
        raw_desc.pop()

        desc = "".join(raw_desc)
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
            self._read_bytes(
                self.config['rtt_data_channel'],
                1),
            byteorder=self.config['byteorder'],
            signed=False)
        et = self.received_events.registered_events_types[id]

        buf = self._read_bytes(self.config['rtt_data_channel'], 4)
        timestamp_raw = (
            int.from_bytes(
                buf,
                byteorder=self.config['byteorder'],
                signed=False))

        if self.after_half and timestamp_raw < 0.2 * self.config['timestamp_raw_max']:
            self.timestamp_overflows += 1
            self.after_half = False

        if timestamp_raw > 0.6 * \
                self.config['timestamp_raw_max'] and timestamp_raw < 0.9 * self.config['timestamp_raw_max']:
            self.after_half = True

        timestamp = self._calculate_timestamp_from_clock_ticks(timestamp_raw)

        data = []
        for i in et.data_types:
            signum = False
            if i[0] == 's':
                signum = True
            buf = self._read_bytes(self.config['rtt_data_channel'], 4)
            data.append(int.from_bytes(buf, byteorder=self.config['byteorder'],
                                       signed=signum))
        return Event(id, timestamp, data)

    def read_events_rtt(self, time_seconds):
        self.start_logging_events()
        start_time = time.time()
        current_time = start_time
        while current_time - start_time < time_seconds or time_seconds < 0:
            event = self._read_single_event_rtt()
            self.received_events.events.append(event)
            if self.queue is not None:
                self.queue.put(event)
            current_time = time.time()
        self.stop_logging_events()

    def start_logging_events(self):
        self._send_command(Command.START)

    def stop_logging_events(self):
        self._send_command(Command.STOP)

    def _send_command(self, command_type):
        command = bytearray(1)
        command[0] = command_type.value
        self.jlink.rtt_write(self.config['rtt_command_channel'], command, None)
