#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from multiprocessing import Pipe

class StreamError(Exception):
    ERROR_MSG = 'error'
    TIMEOUT_MSG = 'timeout'

class Stream():
    RECV_BUF_SIZE = 2**13

    @staticmethod
    def create_stream(num):
        assert num > 1
        streams = []
        pipes_desc_pairs = []
        pipes_ev_pairs = []
        for i in range(num - 1):
            pipes_desc_pairs.append(Pipe(False))
            pipes_ev_pairs.append(Pipe(False))
        for i in range(num):
            if i == 0:
                streams.append(Stream(pipe_send_desc=pipes_desc_pairs[0][1],
                                      pipe_send_ev=pipes_ev_pairs[0][1]))
            elif i == num - 1:
                streams.append(Stream(pipe_recv_desc=pipes_desc_pairs[-1][0],
                                      pipe_recv_ev=pipes_ev_pairs[-1][0]))
            else:
                streams.append(Stream(pipe_recv_desc=pipes_desc_pairs[i - 1][0],
                                      pipe_recv_ev=pipes_ev_pairs[i - 1][0],
                                      pipe_send_desc=pipes_desc_pairs[i][1],
                                      pipe_send_ev=pipes_ev_pairs[i][1]))

        return streams

    def __init__(self, pipe_recv_desc=None, pipe_recv_ev=None, timeouts=None,
                 pipe_send_desc=None, pipe_send_ev=None):
        self.pipe_recv_desc = pipe_recv_desc
        self.pipe_recv_ev = pipe_recv_ev

        if timeouts is None:
            self.timeouts = {
                'descriptions': None,
                'events': None
            }
        else:
            self.timeouts = timeouts

        self.pipe_send_desc = pipe_send_desc
        self.pipe_send_ev = pipe_send_ev

    def set_timeouts(self, timeouts):
        self.timeouts = timeouts

    @staticmethod
    def _send(bytes, pipe):
        if pipe is None:
            raise StreamError('Pipe for sending is not created.', StreamError.ERROR_MSG)
        # Ensure that data would not be dropped.
        assert len(bytes) <= Stream.RECV_BUF_SIZE
        try:
            pipe.send_bytes(bytes)
        except ValueError as err:
            raise StreamError(err, StreamError.ERROR_MSG)

    def send_desc(self, bytes):
        self._send(bytes, self.pipe_send_desc)

    def send_ev(self, bytes):
        self._send(bytes, self.pipe_send_ev)

    @staticmethod
    def _receive(pipe, timeout):
        if pipe is None:
            raise StreamError('Pipe for receiving is not created.', StreamError.ERROR_MSG)
        if pipe.poll(timeout):
            try:
                bytes = pipe.recv_bytes(Stream.RECV_BUF_SIZE)
                return bytes
            except (EOFError, OSError) as err:
                raise StreamError(err, StreamError.ERROR_MSG)
        else:
            raise StreamError('Timeout reached on receiving end of pipe.', StreamError.TIMEOUT_MSG)

    def recv_desc(self):
        return self._receive(self.pipe_recv_desc, self.timeouts['descriptions'])

    def recv_ev(self):
        return self._receive(self.pipe_recv_ev, self.timeouts['events'])
