#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from rtt_nordic_config import RttNordicConfig
import sys
import logging
import time
from pynrfjprog.LowLevel import API
from pynrfjprog.APIError import APIError
from enum import Enum
from stream import StreamError

class Command(Enum):
    START = 1
    STOP = 2
    INFO = 3

class Rtt2Stream:
    def __init__(self, out_stream, event_close, config=RttNordicConfig, log_lvl=logging.INFO):
        self.config = config

        self.out_stream = out_stream

        self.event_close = event_close

        self.logger = logging.getLogger('Profiler Rtt to stream')
        self.logger_console = logging.StreamHandler()
        self.logger.setLevel(log_lvl)
        self.log_format = logging.Formatter('[%(levelname)s] %(name)s: %(message)s')
        self.logger_console.setFormatter(self.log_format)
        self.logger.addHandler(self.logger_console)

        self.rtt_up_channels = {
            'info': None,
            'data': None,
        }
        self.rtt_down_channels = {
            'command': None,
        }
        self._connect_rtt()

    @staticmethod
    def _rtt_get_device_family(snr):
        family = None
        with API('UNKNOWN') as api:
            if snr is not None:
                api.connect_to_emu_with_snr(snr)
            else:
                api.connect_to_emu_without_snr()
            family = api.read_device_family()
            api.disconnect_from_emu()
        return family

    def _connect_rtt(self):
        snr = self.config['device_snr']
        device_family = Rtt2Stream._rtt_get_device_family(snr)
        self.logger.info('Recognized device family: ' + device_family)
        self.jlink = API(device_family)
        self.jlink.open()

        if snr is not None:
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

            time.sleep(0.2)

        while (None in list(self.rtt_up_channels.values())) or \
              (None in list(self.rtt_down_channels.values())):
            down_channel_cnt, up_channel_cnt = self.jlink.rtt_read_channel_count()

            for idx in range(0, down_channel_cnt):
                chan_name, _ = self.jlink.rtt_read_channel_info(idx, 'DOWN_DIRECTION')

                try:
                    label = self.config['rtt_down_channel_names'][chan_name]
                    self.rtt_down_channels[label] = idx
                except KeyError:
                    continue

            for idx in range(0, up_channel_cnt):
                chan_name, _ = self.jlink.rtt_read_channel_info(idx, 'UP_DIRECTION')

                try:
                    label = self.config['rtt_up_channel_names'][chan_name]
                    self.rtt_up_channels[label] = idx
                except KeyError:
                    continue

            if time.time() - start_time > TIMEOUT:
                self.logger.error("Cannot find properly configured RTT channels")
                sys.exit()

            time.sleep(0.2)

        self.logger.info("Connected to device via RTT")

    def _read_remaining_rtt_data(self):
        # Read remaining data from device and send it.
        self._stop_logging_events()

        buf = self._read_bytes()
        while len(buf) > 0:
            try:
                self.out_stream.send_ev(buf)
            except StreamError as err:
                self.logger.error("Error: {}. Unable to send remaining data".format(err))
                break
            buf = self._read_bytes()

    def _disconnect_rtt(self):
        try:
            self.jlink.rtt_stop()
            self.jlink.disconnect_from_emu()
            self.jlink.close()

        except APIError:
            self.logger.error("JLink connection lost")
            return

        self.logger.info("Disconnected from device")

    def _read_bytes(self):
        try:
            buf = self.jlink.rtt_read(self.rtt_up_channels['data'],
                                      self.config['rtt_read_chunk_size'],
                                      encoding=None)

        except APIError:
            self.logger.error("Problem with reading RTT data")
            self._disconnect_rtt()
            sys.exit()

        return buf

    def _read_all_events_descriptions(self):
        self._send_command(Command.INFO)
        desc_buf = bytearray()
        # Empty field is sent after last event description
        while True:
            if self.event_close.is_set():
                self.logger.info("Module closed before receiving event descriptions.")
                self._disconnect_rtt()
                sys.exit()

            try:
                buf_temp = self.jlink.rtt_read(self.rtt_up_channels['info'],
                                               self.config['rtt_read_chunk_size'],
                                               encoding=None)
            except APIError:
                self.logger.error("Problem with reading RTT data")
                self._disconnect_rtt()
                sys.exit()

            desc_buf.extend(buf_temp)
            if buf_temp[-2:] == bytearray('\n\n', 'utf-8'):
                return desc_buf
            time.sleep(0.1)

    def read_and_transmit_data(self):
        desc_buf = self._read_all_events_descriptions()
        try:
            self.out_stream.send_desc(desc_buf)
        except StreamError as err:
            self.logger.error("Error: {}. Unable to send data".format(err))
            self._disconnect_rtt()
            sys.exit()

        self._start_logging_events()
        while True:
            if self.event_close.is_set():
                self.close()

            buf = self._read_bytes()

            if len(buf) > 0:
                try:
                    self.out_stream.send_ev(buf)
                except StreamError as err:
                    self.logger.error("Error: {}. Unable to send data".format(err))
                    self._disconnect_rtt()
                    sys.exit()

            if len(buf) < self.config['rtt_additional_read_thresh']:
                time.sleep(self.config['rtt_read_sleep_time'])

    def _start_logging_events(self):
        self._send_command(Command.START)

    def _stop_logging_events(self):
        self._send_command(Command.STOP)

    def _send_command(self, command_type):
        command = bytearray(1)
        command[0] = command_type.value
        try:
            self.jlink.rtt_write(self.rtt_down_channels['command'], command, None)
        except APIError:
            self.logger.error("Problem with writing RTT data")

    def close(self):
        self.logger.info("Real time transmission closed")
        self._read_remaining_rtt_data()
        self._disconnect_rtt()
        sys.exit()
