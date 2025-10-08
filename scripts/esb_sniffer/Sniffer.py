#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import logging
import struct
from enum import Enum
from time import sleep

from pynrfjprog.APIError import APIError
from pynrfjprog.LowLevel import API
from pynrfjprog.Parameters import CoProcessor


class RttCommands(Enum):
    SNIFFER_START  = 1
    SNIFFER_STOP   = 2
    SNIFFER_STATUS = 3

class Sniffer:
    '''API to communicate with the DK.'''
    def __init__(self, packet_len: int=45, rtt_channels: dict=None, read_at_once: int=2000, swd_freq_khz: int=8000, log_lvl=logging.INFO):
        if rtt_channels is None:
            rtt_channels = {"data_down": 1, "comm_down": 2, "comm_up": 1}
        self.jlink = None
        self.remaining_data = None
        self.packet_len = packet_len
        self.rtt_ch = rtt_channels
        self.read_at_once = read_at_once
        self.swd_freq_khz = swd_freq_khz
        self.logger = logging.getLogger("ESBSniffer")
        self.logger.setLevel(log_lvl)
        logging.basicConfig()

    def __get_packet_length(self) -> int:
        '''Get packet length from the DK.'''
        timeout = 2
        try:
            # wait for device to settle packet_length in rtt buffer,
            # because rrt_read is no blocking and can read 0 instead
            sleep(0.4)
            ret = self.jlink.rtt_read(self.rtt_ch["comm_down"], 4, encoding=None)
            while (len(ret) == 0 and timeout > 0):
                sleep(0.2)
                timeout -= 0.2
                ret = self.jlink.rtt_read(self.rtt_ch["comm_down"], 4, encoding=None)
        except APIError as err:
            self.logger.error("Failed to get packet length from sniffer: err = %d, using default %d", err.err_code, self.packet_len)
            return -1

        if len(ret) == 4:
            length = int.from_bytes(ret, byteorder='big', signed=False)
        else:
            self.logger.error("Failed to get packet length from sniffer, using default %d", self.packet_len)
            return -1

        if length != 0:
            self.packet_len = length

        return 0

    def connect(self) -> int:
        '''Connect to the DK.'''
        timeout = 5

        if self.jlink is not None:
            return 0

        try:
            self.jlink = API()
            self.jlink.open()
            self.jlink.connect_to_emu_without_snr(jlink_speed_khz=self.swd_freq_khz)

            if self.jlink.read_device_family() == "NRF53":
                self.jlink.select_coprocessor(CoProcessor.CP_NETWORK)

            self.jlink.sys_reset()
            self.jlink.go()
            self.jlink.rtt_start()
        except APIError as err:
            self.logger.error("Failed to connect to device: err = %d", err.err_code)
            self.jlink = None
            return -1

        while (not self.jlink.rtt_is_control_block_found() and timeout > 0):
            sleep(1)
            timeout -= 1

        if timeout == 0:
            self.logger.error("Failed to find rtt control block")
            self.jlink = None
            return -1


        self.__get_packet_length()
        self.logger.info("Connected to device")
        return 0

    def disconnect(self):
        '''Disconnect from the DK.'''
        try:
            self.jlink.close()
        except APIError:
            self.logger.error("JLink connection lost")

        self.jlink = None
        self.logger.info("Disconnected from device")

    def __gen_pcap_packets(self, buff: bytearray) -> list:
        '''Parse data from DK into pcap formatted packets.'''
        if len(buff) < self.packet_len:
            return b''

        packets = []

        # Since rtt_read(self.packet_len) can read fragment of the packet,
        # ensure that we will parse it correctly
        if self.remaining_data is not None:
            buff = self.remaining_data + buff
            self.remaining_data = None

        if len(buff) % self.packet_len != 0:
            start = int(len(buff)/self.packet_len) * self.packet_len
            self.remaining_data = buff[start:]

        for i in range(0, int(len(buff)/self.packet_len)):
            pkt = buff[i*self.packet_len:(i+1)*self.packet_len]
            if pkt != b'':
                timestamp_ms = int.from_bytes(pkt[0:4], byteorder='big', signed=False)
                timestamp_us = int.from_bytes(pkt[4:8], byteorder='big', signed=False)

                sec = int(timestamp_ms / 1000)
                usec = int(timestamp_us+((timestamp_ms % 1000) * 1e3))
                data_length = pkt[8]
                pkt_len = 5 + data_length

                pkt_header = struct.pack('<IIII', sec, usec, pkt_len, pkt_len)
                packets.append(pkt_header + pkt[8:8+pkt_len])

        return packets

    def __send_command(self, command: RttCommands) -> int:
        '''Send command to the DK through RTT.'''
        bin_command = struct.pack("B", command.value)

        try:
            self.jlink.rtt_write(self.rtt_ch["comm_up"], bin_command, None)

            timeout = 2
            ret = self.jlink.rtt_read(self.rtt_ch["comm_down"], 4, encoding=None)
            while (len(ret) == 0 and timeout > 0):
                sleep(0.2)
                timeout -= 0.2
                ret = self.jlink.rtt_read(self.rtt_ch["comm_down"], 4, encoding=None)

            if len(ret) == 4:
                ret = int.from_bytes(ret[0:4], byteorder='big', signed=False)
                return ret
            else:
                self.logger.error("Connection timeout")
                return -1
        except APIError as err:
            self.logger.error("Jlink error while sending command, %d", err.err_code)
            return -1

    def start(self) -> int:
        '''Send start command to the DK.'''
        if self.jlink is None:
            raise OSError

        ret = self.__send_command(RttCommands.SNIFFER_START)

        if ret < 0:
            self.logger.error("Failed to start sniffer")
            return -1

        self.logger.info("Sniffer started")
        return 0

    def stop(self) -> int:
        '''Send stop command to the DK.'''
        if self.jlink is None:
            return ValueError("JLink not initialized")

        ret = self.__send_command(RttCommands.SNIFFER_STOP)

        if ret < 0:
            self.logger.error("Failed to stop sniffer, please restart device to clear receive buffer")
            return -1

        self.logger.info("Sniffer stopped")
        return 0

    def get_status(self) -> str:
        '''Send status command to the DK.'''
        if self.jlink is None:
            raise OSError

        ret = self.__send_command(RttCommands.SNIFFER_STATUS)

        if ret < 0:
            self.logger.error("Failed to check sniffer status")
            return -1

        if ret == 0:
            return "Stopped"
        else:
            return "Running"

    def read(self) -> bytearray:
        '''Read data from the DK.'''
        if self.jlink is None:
            raise OSError

        try:
            buff = self.jlink.rtt_read(self.rtt_ch["data_down"], self.read_at_once*self.packet_len, encoding=None)
        except APIError as err:
            self.logger.error("JLink connection error while reading packets: err = %d", err.err_code)
            raise OSError

        packets = self.__gen_pcap_packets(buff)

        return bytearray(b''.join([bytes(i) for i in packets]))

    def _get_com_port(self) -> 'str|None':
        '''Find serial ports of the connected DK.'''
        if self.jlink is None:
            raise OSError

        try:
            sn = self.jlink.read_connected_emu_snr()
            family = self.jlink.read_device_family()
            ports = self.jlink.enum_emu_com_ports(sn)
            if len(ports) == 0:
                self.logger.info("Device does not expose serial ports")
                return None

            if family in ("NRF54L", "NRF54H"):
                return ports[0].path
            elif family == "NRF53":
                return ports[1].path
            elif family == "NRF52":
                return ports[0].path
            else:
                self.logger.error("Unsupported device family!")
                return None

        except APIError:
            self.logger.error("Failed to read available com ports")
            return None
