#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import socket
import errno

class StreamError(Exception):
    pass

class Stream():
    RECV_BUF_SIZE = 2**13

    def __init__(self, own_socket_dict, timeouts, remote_socket_dict=None):
        self.own_sock_desc = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
        self.own_sock_desc.bind(own_socket_dict['descriptions'])
        self.own_sock_desc.settimeout(timeouts['descriptions'])
        self.own_sock_ev = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
        self.own_sock_ev.bind(own_socket_dict['events'])
        self.own_sock_ev.settimeout(timeouts['events'])

        self.remote_socket_dict = remote_socket_dict

    @staticmethod
    def _send(bytes, own_socket, remote_address):
        try:
            own_socket.sendto(bytes, remote_address)
        except socket.error as err:
            if err.errno == errno.ECONNREFUSED:
                raise StreamError(err, 'closed')
            else:
                raise StreamError(err, 'error')

    def send_desc(self, bytes):
        if self.remote_socket_dict is not None:
            self._send(bytes, self.own_sock_desc, self.remote_socket_dict['descriptions'])
    def send_ev(self, bytes):
        if self.remote_socket_dict is not None:
            self._send(bytes, self.own_sock_ev, self.remote_socket_dict['events'])

    @staticmethod
    def _receive(own_socket):
        try:
            bytes, _ = own_socket.recvfrom(Stream.RECV_BUF_SIZE)
            return bytes
        except socket.error as err:
            if err.errno == errno.EAGAIN or err.errno == errno.EWOULDBLOCK:
                raise StreamError(err, 'timeout')
            else:
                raise StreamError(err, 'error')

    def recv_desc(self):
        return self._receive(self.own_sock_desc)

    def recv_ev(self):
        return self._receive(self.own_sock_ev)
