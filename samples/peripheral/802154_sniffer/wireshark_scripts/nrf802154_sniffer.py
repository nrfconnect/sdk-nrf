#!/usr/bin/env python

# Copyright (c) 2019, Nordic Semiconductor ASA
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form, except as embedded into a Nordic
#    Semiconductor ASA integrated circuit in a product or a software update for
#    such product, must reproduce the above copyright notice, this list of
#    conditions and the following disclaimer in the documentation and/or other
#    materials provided with the distribution.
#
# 3. Neither the name of Nordic Semiconductor ASA nor the names of its
#    contributors may be used to endorse or promote products derived from this
#    software without specific prior written permission.
#
# 4. This software, with or without modification, must only be used with a
#    Nordic Semiconductor ASA integrated circuit.
#
# 5. Any software provided in binary form under this license must not be reverse
#    engineered, decompiled, modified and/or disassembled.
#
# THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import sys
import os

is_standalone = __name__ == '__main__'

if sys.version[0] == '2':
    # py2 support
    import Queue as Queue
else:
    import queue as Queue

if is_standalone:
    sys.path.insert(0, os.getcwd())

import re
import signal
import struct
import threading
import time
import logging
from argparse import ArgumentParser
from binascii import a2b_hex
from serial import Serial, serialutil
from serial.tools.list_ports import comports


class Nrf802154Sniffer(object):

    # Various options for pcap files: http://www.tcpdump.org/linktypes.html
    DLT_USER0 = 147
    DLT_IEEE802_15_4_NOFCS = 230
    DLT_IEEE802_15_4_TAP = 283

    # USB device identification.
    NORDICSEMI_VID = 0x1915
    SNIFFER_802154_PID = 0x154B

    # Helpers for Wireshark argument parsing.
    CTRL_ARG_CHANNEL = 0
    CTRL_ARG_LOGGER = 6

    # Pattern for packets being printed over serial.
    RCV_REGEX = 'received:\s+([0-9a-fA-F]+)\s+power:\s+(-?\d+)\s+lqi:\s+(\d+)\s+time:\s+(-?\d+)'

    TIMER_MAX = 2**32

    def __init__(self, connection_open_timeout=None):
        self.serial = None
        self.serial_queue = Queue.Queue()
        self.running = threading.Event()
        self.setup_done = threading.Event()
        self.setup_done.clear()
        self.logger = logging.getLogger(__name__)
        self.dev = None
        self.channel = None
        self.dlt = None
        self.threads = []
        self.connection_open_timeout = connection_open_timeout

        # Time correction variables.
        self.first_local_timestamp = None
        self.first_sniffer_timestamp = None

    def correct_time(self, sniffer_timestamp):
        """
        Function responsible for correcting the time reported by the sniffer.
        The sniffer timer has 1us resolution and will overflow after
        approximately every 72 minutes of operation.
        For that reason it is necessary to use the local system timer to find the absolute
        time frame, within which the packet has arrived.

        This method should work as long as the MCU and PC timers don't drift
        from each other by a value of approximately 36 minutes.

        :param sniffer_timestamp: sniffer timestamp in microseconds
        :return: absolute sniffer timestamp in microseconds
        """
        if self.first_local_timestamp is None:
            # First received packets - set the reference time and convert to microseconds.
            self.first_local_timestamp = int(time.time()*(10**6))
            self.first_sniffer_timestamp = sniffer_timestamp
            return self.first_local_timestamp
        else:
            local_timestamp = int(time.time()*(10**6))
            time_difference = local_timestamp - self.first_local_timestamp

            # Absolute sniffer timestamp calculated locally
            absolute_sniffer_timestamp = self.first_sniffer_timestamp + time_difference

            overflow_count = absolute_sniffer_timestamp // self.TIMER_MAX
            timestamp_modulo = absolute_sniffer_timestamp % self.TIMER_MAX

            # Handle the corner case - sniffer timer is about to overflow.
            # Due to drift the locally calculated absolute timestamp reports that the overflow
            # has already occurred. If the modulo of calculated time with added value of half timer period
            # is smaller than the sniffer timestamp, then we decrement the overflow counter.
            #
            # The second corner case is when the sniffer timestamp just overflowed and the value is close to zero.
            # Locally calculated timestamp reports that the overflow hasn't yet occurred. We ensure that this is the
            # case by testing if the sniffer timestamp is less than modulo of calculated timestamp substracted by
            # half of timer period. In this case we increment overflow count.
            if (timestamp_modulo + self.TIMER_MAX//2) < sniffer_timestamp:
                overflow_count -= 1
            elif (timestamp_modulo - self.TIMER_MAX//2) > sniffer_timestamp:
                overflow_count += 1

            return self.first_local_timestamp - self.first_sniffer_timestamp + sniffer_timestamp + overflow_count * self.TIMER_MAX

    def stop_sig_handler(self, *args, **kwargs):
        """
        Function responsible for stopping the sniffer firmware and closing all threads.
        """
        # Let's wait with closing afer we're sure that the sniffer started. Protects us
        # from very short tests (NOTE: the serial_reader has a delayed start).
        while self.running.is_set() and not self.setup_done.is_set():
            time.sleep(1)

        if self.running.is_set():
            self.serial_queue.put(b'')
            self.serial_queue.put(b'sleep')
            self.running.clear()

            alive_threads = []

            for thread in self.threads:
                try:
                    thread.join(timeout=10)
                    if thread.is_alive() is True:
                        self.logger.error("Failed to stop thread {}".format(thread.name))
                        alive_threads.append(thread)
                except RuntimeError:
                    # TODO: This may be called from one of threads from thread list - architecture problem
                    pass

            self.threads = alive_threads
        else:
            self.logger.warning("Asked to stop {} while it was already stopped".format(self))

        if self.serial is not None:
            if self.serial.is_open is True:
                self.serial.close()
            self.serial = None

    @staticmethod
    def extcap_interfaces():
        """
        Wireshark-related method that returns configuration options.
        :return: string with wireshark-compatible information
        """
        res = []
        res.append("extcap {version=0.7.2}{help=https://github.com/NordicSemiconductor/nRF-Sniffer-for-802.15.4}{display=nRF Sniffer for 802.15.4}")
        for port in comports():
            if port.vid == Nrf802154Sniffer.NORDICSEMI_VID and port.pid == Nrf802154Sniffer.SNIFFER_802154_PID:
                res.append ("interface {value=%s}{display=nRF Sniffer for 802.15.4}" % (port.device,) )

        res.append("control {number=%d}{type=button}{role=logger}{display=Log}{tooltip=Show capture log}" % Nrf802154Sniffer.CTRL_ARG_LOGGER)

        return "\n".join(res)

    @staticmethod
    def extcap_dlts():
        """
        Wireshark-related method that returns configuration options.
        :return: string with wireshark-compatible information
        """
        res = []
        res.append("dlt {number=%d}{name=IEEE802_15_4_NOFCS}{display=IEEE 802.15.4 without FCS}" % Nrf802154Sniffer.DLT_IEEE802_15_4_NOFCS)
        res.append("dlt {number=%d}{name=IEEE802_15_4_TAP}{display=IEEE 802.15.4 TAP}" % Nrf802154Sniffer.DLT_IEEE802_15_4_TAP)
        res.append("dlt {number=%d}{name=USER0}{display=User 0 (DLT=147)}" % Nrf802154Sniffer.DLT_USER0)

        return "\n".join(res)

    @staticmethod
    def extcap_config(option):
        """
        Wireshark-related method that returns configuration options.
        :return: string with wireshark-compatible information
        """
        args = []
        values = []
        res =[]

        args.append ( (0, '--channel', 'Channel', 'IEEE 802.15.4 channel', 'selector', '{required=true}{default=11}') )
        args.append ( (1, '--metadata', 'Out-Of-Band meta-data',
                          'Packet header containing out-of-band meta-data for channel, RSSI and LQI',
                          'selector', '{default=none}') )

        if len(option) <= 0:
            for arg in args:
                res.append("arg {number=%d}{call=%s}{display=%s}{tooltip=%s}{type=%s}%s" % arg)

            values = values + [ (0, "%d" % i, "%d" % i, "true" if i == 11 else "false" ) for i in range(11,27) ]

            values.append ( (1, "none", "None", "true") )
            values.append ( (1, "ieee802154-tap", "IEEE 802.15.4 TAP", "false") )
            values.append ( (1, "user", "Custom Lua dissector", "false") )

        for value in values:
            res.append("value {arg=%d}{value=%s}{display=%s}{default=%s}" % value)
        return "\n".join(res)

    def pcap_header(self):
        """
        Returns pcap header to be written into pcap file.
        """
        header = bytearray()
        header += struct.pack('<L', int ('a1b2c3d4', 16 ))
        header += struct.pack('<H', 2 ) # Pcap Major Version
        header += struct.pack('<H', 4 ) # Pcap Minor Version
        header += struct.pack('<I', int(0)) # Timezone
        header += struct.pack('<I', int(0)) # Accurancy of timestamps
        header += struct.pack('<L', int ('000000ff', 16 )) # Max Length of capture frame
        header += struct.pack('<L', self.dlt) # DLT
        return header

    @staticmethod
    def pcap_packet(frame, dlt, channel, rssi, lqi, timestamp):
        """
        Creates pcap packet to be seved in pcap file.
        """
        pcap = bytearray()

        caplength = len(frame)

        if dlt == Nrf802154Sniffer.DLT_IEEE802_15_4_TAP:
            caplength += 28
        elif dlt == Nrf802154Sniffer.DLT_USER0:
            caplength += 6

        pcap += struct.pack('<L', timestamp // 1000000 ) # Timestamp seconds
        pcap += struct.pack('<L', timestamp % 1000000 ) # Timestamp microseconds
        pcap += struct.pack('<L', caplength ) # Length captured
        pcap += struct.pack('<L', caplength ) # Length in frame

        if dlt == Nrf802154Sniffer.DLT_IEEE802_15_4_TAP:
            # Append TLVs according to 802.15.4 TAP specification:
            # https://github.com/jkcko/ieee802.15.4-tap
            pcap += struct.pack('<HH', 0, 28)
            pcap += struct.pack('<HHf', 1, 4, rssi)
            pcap += struct.pack('<HHHH', 3, 3, channel, 0)
            pcap += struct.pack('<HHI', 10, 1, lqi)
        elif dlt == Nrf802154Sniffer.DLT_USER0:
            pcap += struct.pack('<H', channel)
            pcap += struct.pack('<h', rssi)
            pcap += struct.pack('<H', lqi)

        pcap += frame

        return pcap

    @staticmethod
    def control_read(fn):
        """
        Method used for reading wireshark command.
        """
        try:
            header = fn.read(6)
            sp, _, length, arg, typ = struct.unpack('>sBHBB', header)
            if length > 2:
                payload = fn.read(length - 2)
            else:
                payload = ''
            return arg, typ, payload
        except:
            return None, None, None

    def control_reader(self, fifo):
        """
        Thread responsible for reading wireshark commands (read from fifo).
        Related to not-yet-implemented wireshark toolbar features.
        """
        with open(fifo, 'rb', 0) as fn:
            arg = 0
            while arg != None:
                arg, typ, payload = Nrf802154Sniffer.control_read(fn)
            self.stop_sig_handler()

    def is_running(self):
        return self.serial is not None and self.serial.is_open and self.setup_done.is_set()

    def serial_write(self):
        """
        Function responsible for sending commands to serial port.
        """
        command = self.serial_queue.get(block=True, timeout=1)
        try:
            self.serial.write(command + b'\r\n')
            # The shell will add this \r\n for us
            #self.serial.write(b'\r\n')
        except IOError:
            self.logger.error("Cannot write to {}".format(self))
            self.running.clear()

    def serial_writer(self):
        """
        Thread responsible for sending commands to serial port.
        """
        while self.running.is_set():
            try:
                self.serial_write()
            except Queue.Empty:
                pass

        # Write final commands and break out.
        while True:
            try:
                self.serial_write()
            except Queue.Empty:
                break

    def serial_reader(self, dev, channel, queue):
        """
        Thread responsible for reading from serial port, parsing the output and storing parsed packets into queue.
        """
        time.sleep(2)

        timeout = time.time() + self.connection_open_timeout if self.connection_open_timeout else None
        while self.running.is_set():
            try:
                self.serial = Serial(dev, timeout=1, exclusive=True)
                break
            except Exception as e:
                if timeout and time.time() > timeout:
                    self.running.clear()
                    raise Exception(
                        "Could not open serial connection to sniffer before timeout of {} seconds".format(
                            self.connection_open_timeout))
                self.logger.debug("Can't open serial device: {} reason: {}".format(dev, e))
                time.sleep(0.5)

        try:
            self.serial.reset_input_buffer()
            self.serial.reset_output_buffer()

            # Throw out the first /r/n on connect
            self.serial.read(2)

            init_cmd = []
            init_cmd.append(b'')
            init_cmd.append(b'sleep')
            init_cmd.append(b'channel ' + bytes(str(channel).encode()))
            for cmd in init_cmd:
                self.serial_queue.put(cmd)

            # Function serial_write appends twice '\r\n' to each command, so we have to calculate that for the echo.
            init_res = self.serial.read(len(b"".join(c + b"\r\n\r\n" for c in init_cmd)))

            if not all(cmd.decode() in init_res.decode() for cmd in init_cmd):
                msg = "{} did not reply properly to setup commands. Please re-plug the device and make sure firmware is correct. " \
                        "Recieved: {}\n".format(self, init_res)
                if self.serial.is_open is True:
                    self.serial.close()
                raise Exception(msg)

            self.serial_queue.put(b'receive')
            self.setup_done.set()

            buf = b''

            while self.running.is_set():
                ch = self.serial.read()
                if ch == b'':
                    continue
                elif ch != b'\n' and ch != '\n':
                    buf += ch
                else:
                    m = re.search(self.RCV_REGEX, str(buf))
                    if m:
                        packet = a2b_hex(m.group(1)[:-4])
                        rssi = int(m.group(2))
                        lqi = int(m.group(3))
                        timestamp = int(m.group(4)) & 0xffffffff
                        channel = int(channel)
                        queue.put(self.pcap_packet(packet, self.dlt, channel, rssi, lqi, self.correct_time(timestamp)))
                    buf = b''

        except (serialutil.SerialException, serialutil.SerialTimeoutException) as e:
            self.logger.error("Cannot communicate with serial device: {} reason: {}".format(dev, e))
        finally:
            self.setup_done.set()  # In case it wasn't set before.
            if self.running.is_set():  # Another precaution.
                self.stop_sig_handler()

    def fifo_writer(self, fifo, queue):
        """
        Thread responsible for writing packets into pcap file/fifo from queue.
        """
        with open(fifo, 'wb', 0 ) as fh:
            fh.write(self.pcap_header())
            fh.flush()

            while self.running.is_set():
                try:
                    packet = queue.get(block=True, timeout=1)
                    try:
                        fh.write(packet)
                        fh.flush()
                    except IOError:
                        pass

                except Queue.Empty:
                    pass

    def extcap_capture(self, fifo, dev, channel, metadata=None, control_in=None, control_out=None):
        """
        Main method responsible for starting all other threads. In case of standalone execution this method will block
        until SIGTERM/SIGINT and/or stop_sig_handler disables the loop via self.running event.
        """

        if len(self.threads):
            raise RuntimeError("Old threads were not joined properly")

        packet_queue = Queue.Queue()
        self.channel = channel
        self.dev = dev
        self.running.set()

        if metadata == "ieee802154-tap":
            # For Wireshark 3.0 and later
            self.dlt = Nrf802154Sniffer.DLT_IEEE802_15_4_TAP
        elif metadata == "user":
            # For Wireshark 2.4 and 2.6
            self.dlt = Nrf802154Sniffer.DLT_USER0
        else:
            self.dlt = Nrf802154Sniffer.DLT_IEEE802_15_4_NOFCS

        # TODO: Add toolbar with channel selector (channel per interface?)
        if control_in:
            self.threads.append(threading.Thread(target=self.control_reader, args=(control_in,)))

        self.threads.append(threading.Thread(target=self.serial_reader, args=(self.dev, self.channel, packet_queue), name="serial_reader"))
        self.threads.append(threading.Thread(target=self.serial_writer, name="serial_writer"))
        self.threads.append(threading.Thread(target=self.fifo_writer, args=(fifo, packet_queue), name="fifo_writer"))

        for thread in self.threads:
            thread.start()

        while is_standalone and self.running.is_set():
            time.sleep(1)

    @staticmethod
    def parse_args():
        """
        Helper methods to make the standalone script work in console and wireshark.
        """
        parser = ArgumentParser(description="Extcap program for the nRF Sniffer for 802.15.4")

        parser.add_argument("--extcap-interfaces", help="Provide a list of interfaces to capture from", action="store_true")
        parser.add_argument("--extcap-interface", help="Provide the interface to capture from")
        parser.add_argument("--extcap-dlts", help="Provide a list of dlts for the given interface", action="store_true")
        parser.add_argument("--extcap-config", help="Provide a list of configurations for the given interface", action="store_true")
        parser.add_argument("--extcap-reload-option", help="Reload elements for the given option")
        parser.add_argument("--capture", help="Start the capture routine", action="store_true" )
        parser.add_argument("--fifo", help="Use together with capture to provide the fifo to dump data to")
        parser.add_argument("--extcap-capture-filter", help="Used together with capture to provide a capture filter")
        parser.add_argument("--extcap-control-in", help="Used to get control messages from toolbar")
        parser.add_argument("--extcap-control-out", help="Used to send control messages to toolbar")

        parser.add_argument("--channel", help="IEEE 802.15.4 capture channel [11-26]")
        parser.add_argument("--metadata", help="Meta-Data type to use for captured packets")

        result, unknown = parser.parse_known_args()

        if result.capture and not result.extcap_interface:
            parser.error("--extcap-interface is required if --capture is present")

        return result

    def __str__(self):
        return "{} ({}) channel {}".format(type(self).__name__, self.dev, self.channel)

    def __repr__(self):
        return self.__str__()


if is_standalone:
    args = Nrf802154Sniffer.parse_args()

    logging.basicConfig(format='%(asctime)s [%(levelname)s] %(message)s', level=logging.INFO)

    sniffer_comm = Nrf802154Sniffer()

    if args.extcap_interfaces:
        print(sniffer_comm.extcap_interfaces())

    if args.extcap_dlts:
        print(sniffer_comm.extcap_dlts())

    if args.extcap_config:
        if args.extcap_reload_option and len(args.extcap_reload_option) > 0:
            option = args.extcap_reload_option
        else:
            option = ''
        print(sniffer_comm.extcap_config(option))

    if args.capture and args.fifo:
        channel = args.channel if args.channel else 11
        signal.signal(signal.SIGINT, sniffer_comm.stop_sig_handler)
        signal.signal(signal.SIGTERM, sniffer_comm.stop_sig_handler)
        try:
            sniffer_comm.extcap_capture(args.fifo, args.extcap_interface, channel, args.metadata, args.extcap_control_in, args.extcap_control_out)
        except KeyboardInterrupt as e:
            sniffer_comm.stop_sig_handler()

