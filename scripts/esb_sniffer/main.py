#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import contextlib
import logging
import os
import sys
from threading import Event, Thread
from time import sleep

from serial import Serial, SerialException
from Sniffer import Sniffer

PIPE_DATA = "/tmp/esb_sniffer_data"
PIPE_COMM = "/tmp/esb_sniffer_comm"

COMM_START_CAPTURE = "1"
COMM_STOP_CAPTURE  = "2"

LOG_LVL = logging.INFO

class Process:
    '''Base for actual threads.'''
    def __init__(self, name: str, target, daemon: bool=True):
        self.name = name
        self.logger = logging.getLogger(self.name)
        self.logger.setLevel(LOG_LVL)
        logging.basicConfig()
        self.thread = Thread(target=target, daemon=daemon)
        self.stop_evt = Event()
        self.exit_exception = None

    def start(self):
        self.thread.start()

    def stop(self):
        self.stop_evt.set()

class PipeData(Process):
    '''Read packets from device and write into extcap pipe.'''
    def __init__(self, event: Event, dev: Sniffer):
        super().__init__("PipeData", self.loop)
        self.dev = dev
        self.run = event

    def loop(self) -> int:
        self.pipe = open(PIPE_DATA, 'wb')
        try:
            while not self.stop_evt.is_set():
                if self.run.is_set():
                    pkt = self.dev.read()

                    if pkt:
                        try:
                            self.pipe.write(pkt)
                            self.pipe.flush()
                        except BrokenPipeError:
                            if not os.path.exists(PIPE_DATA):
                                # removed pipe, cannot safetly continue
                                raise BrokenPipeError
                else:
                    self.run.wait(timeout=30)
        except Exception as e:
            self.exit_exception = type(e).__name__
            return 0

        return 0

    def __del__(self):
        with contextlib.suppress(Exception):
            self.pipe.close()

class PipeComm(Process):
    '''Start and stop device in response from extcap.'''
    def __init__(self, data_event: Event, dev: Sniffer):
        super().__init__("PipeComm", self.loop)
        self.data_evt = data_event
        self.dev = dev

    def loop(self) -> int:
        try:
            self.pipe = open(PIPE_COMM)

            while not self.stop_evt.is_set():
                comm = self.pipe.read(len(COMM_START_CAPTURE))
                if comm == COMM_START_CAPTURE:
                    if self.dev.start() == 0:
                        self.data_evt.set()
                elif comm == COMM_STOP_CAPTURE:
                    self.data_evt.clear()
                    if self.dev.stop() != 0:
                        pass
                else:
                    sleep(1)

        except Exception as e:
            self.data_evt.clear()
            self.exit_exception = type(e).__name__
            return 0

        return 0

    def __del__(self):
        self.data_evt.clear()
        with contextlib.suppress(Exception):
            self.pipe.close()

class Watchdog(Process):
    '''Watch threads and gracefully stop them in case of error.'''
    def __init__(self, tick: float=0.5):
        super().__init__("Watchdog", self.loop)
        self.tick = tick
        self.watching_threads = []

    def add_thread(self, t):
        '''Add thread to watching list.'''
        self.watching_threads.append(t)

    def stop_all(self):
        '''Stop all watched threads.'''
        if not self.thread.is_alive():
            return -1

        for i in self.watching_threads:
            if i.thread.is_alive():
                self.logger.debug("Thread: %s stopped", i.name)
                i.stop_evt.set()
                i.thread.join(1)

        self.logger.info("Stopped all threads")
        self.stop_evt.set()

    def start_all(self):
        '''Start all watched threads.'''
        if not self.thread.is_alive():
            return -1

        for i in self.watching_threads:
            self.logger.debug("Thread: %s started", i.name)
            i.start()

    def loop(self) -> int:
        self.start_all()
        while not self.stop_evt.is_set():
            for i in self.watching_threads:
                if i.thread.is_alive() is False:
                    if i.exit_exception is not None:
                        self.logger.info("Thread: %s is down, reason: %s", i.name, i.exit_exception)

                    self.stop_all()

            sleep(self.tick)

        self.logger.debug("Thread: %s stopped", self.name)
        return 0

class Shell(Process):
    def __init__(self, com: Serial):
        super().__init__("Shell", self.loop)
        self.com = com

    def loop(self):
        try:
            while not self.stop_evt.is_set():
                data = self.com.read(1)
                if data:
                    sys.stdout.buffer.write(data)
                    sys.stdout.flush()
        except Exception as e:
            self.stop()
            self.exit_exception = type(e).__name__

def main():
    data_event = Event()
    dev = Sniffer()
    use_shell = False

    if not os.path.exists(PIPE_DATA):
        os.mkfifo(PIPE_DATA)
    if not os.path.exists(PIPE_COMM):
        os.mkfifo(PIPE_COMM)

    if dev.connect():
        sys.exit(1)

    com_path = dev._get_com_port()
    if com_path is not None:
        try:
            com = Serial(com_path, 115200)
            use_shell = True
        except SerialException as err:
            print(f"Failed to open shell, {err}")

    wd = Watchdog()
    comm = PipeComm(data_event, dev)
    wd.add_thread(comm)
    data = PipeData(data_event, dev)
    wd.add_thread(data)
    if use_shell:
        shell = Shell(com)
        wd.add_thread(shell)

    wd.start()

    print("Type \"q\" or \"quit\" to stop application")
    while True:
        line = sys.stdin.readline()
        if (line.strip() == "quit" or line.strip() == "q"):
            print("Quitting...")
            break
        else:
            if use_shell:
                com.write(line.encode('utf-8'))

    wd.stop_all()
    os.remove(PIPE_DATA)
    os.remove(PIPE_COMM)
    if use_shell:
        com.close()
    dev.disconnect()
    sys.exit(0)

if __name__ == '__main__':
    main()
