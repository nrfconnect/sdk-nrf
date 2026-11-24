# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Zephyr CAN shell python-can bus with runtime configuration support.

Based on zephyr/tests/drivers/can/host/pytest/can_shell.py, extended with bitrate, mode and
state handling and with the can_host burst/rx counter commands of this test application.
"""

import logging
import re
from collections import deque
from dataclasses import dataclass, field

from can import BusABC, CanProtocol, Message
from can.exceptions import CanOperationError
from can.typechecking import CanFilters
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)

DEFAULT_TIMEOUT = 5.0

RETVAL_REGEX = re.compile(r'^\s*(-?\d+)\s*$')
STATE_REGEX = re.compile(r'^state:\s+(?P<state>\S+)')
RX_ERR_REGEX = re.compile(r'^rx errors:\s+(?P<cnt>\d+)')
TX_ERR_REGEX = re.compile(r'^tx errors:\s+(?P<cnt>\d+)')
STATS_REGEX = re.compile(r'^\s+(?P<name>[a-z0-9 ]+(?:errors|overruns)):\s+(?P<cnt>\d+)')
CAPS_REGEX = re.compile(r'^capabilities:\s+(?P<caps>.*)')
MAX_BITRATE_REGEX = re.compile(r'^max bitrate:\s+(?P<bitrate>\d+)')
BURST_REGEX = re.compile(r'burst done: sent (?P<sent>\d+), failed (?P<failed>\d+)')
RX_COUNT_REGEX = re.compile(
    r'rx count: (?P<count>\d+), gaps: (?P<gaps>\d+), duplicates: (?P<dups>\d+)'
)


@dataclass
class CanControllerState:
    """CAN controller state as reported by 'can show'."""

    state: str
    rx_errors: int
    tx_errors: int
    stats: dict[str, int] = field(default_factory=dict)


class CanShellError(CanOperationError):
    """CAN shell command failure carrying the Zephyr error code."""

    def __init__(self, message: str, error_code: int | None = None) -> None:
        super().__init__(message, error_code)
        self.errno = error_code


class CanShellBus(BusABC):  # pylint: disable=abstract-method
    """
    A CAN interface using the Zephyr CAN shell module.

    Unlike the upstream variant, the controller is not started on construction. Use
    configure() to set mode and bitrates and start the controller.
    """

    def __init__(
        self,
        dut: DeviceAdapter,
        shell: Shell,
        channel: str,
        can_filters: CanFilters | None = None,
        **kwargs,
    ) -> None:
        self._dut = dut
        self._shell = shell
        self._device = channel
        self._is_filtered = False
        self._filter_ids = []
        self._rx_queue: deque[Message] = deque()
        self._frame_regex = re.compile(
            re.escape(self._device)
            + r'\s+(?P<brs>[-B])(?P<esi>[-P])\s+(?P<can_id>[0-9a-f]+)\s+'
            + r'\[(?P<dlc>\d+)\]\s*(?P<data>.*?)\s*$'
        )

        self.channel_info = f'Zephyr CAN shell, device "{self._device}"'
        self.name = channel

        lines = self._exec(f'can show {self._device}')
        self.capabilities = self._match_first(lines, CAPS_REGEX).group('caps').split()
        self.max_bitrate = int(self._match_first(lines, MAX_BITRATE_REGEX).group('bitrate'))

        self._can_protocol = CanProtocol.CAN_20

        self._stop()

        super().__init__(channel=channel, can_filters=can_filters, **kwargs)

    @staticmethod
    def _match_first(lines: list[str], regex: re.Pattern) -> re.Match:
        for line in lines:
            m = regex.search(line)
            if m:
                return m
        raise CanOperationError(f'pattern "{regex.pattern}" not found in shell output')

    def _parse_frame(self, line: str) -> Message | None:
        m = self._frame_regex.search(line)
        if not m:
            return None

        can_id = int(m.group('can_id'), 16)
        ext = len(m.group('can_id')) == 8
        nbytes = int(m.group('dlc'))
        rtr = m.group('data').startswith('remote transmission request')
        data = None if rtr else bytearray.fromhex(m.group('data'))

        msg = Message(
            arbitration_id=can_id,
            is_extended_id=ext,
            is_remote_frame=rtr,
            data=data,
            dlc=nbytes,
            channel=self._device,
            check=True,
        )
        logger.debug('%s received: %s', self._device, msg)
        return msg

    def _collect_frames(self, lines: list[str]) -> None:
        """Keep frames printed while waiting for other output so they are not lost."""
        for line in lines:
            msg = self._parse_frame(line)
            if msg is not None:
                self._rx_queue.append(msg)

    def _exec(self, cmd: str, timeout: float | None = None) -> list[str]:
        lines = self._shell.exec_command(cmd, timeout=timeout)
        self._collect_frames(lines)
        return [line for line in self._shell.get_filtered_output(lines)
                if self._frame_regex.search(line) is None]

    def _retval(self) -> int:
        """Get return value of last shell command."""
        for line in reversed(self._exec('retval')):
            m = RETVAL_REGEX.match(line)
            if m:
                return int(m.group(1))
        raise CanOperationError('retval not found')

    def _exec_checked(self, cmd: str, timeout: float | None = None) -> list[str]:
        lines = self._exec(cmd, timeout=timeout)
        retval = self._retval()
        if retval != 0:
            raise CanShellError(f'"{cmd}" failed (err {retval})', retval)
        return lines

    def _start(self) -> None:
        self._exec_checked(f'can start {self._device}')

    def _stop(self) -> None:
        # -EALREADY is returned when the controller is already stopped
        self._exec(f'can stop {self._device}')

    def start(self) -> None:
        """Start the CAN controller."""
        self._start()

    def stop(self) -> None:
        """Stop the CAN controller."""
        self._stop()

    def configure(
        self,
        bitrate: int,
        mode: str = 'normal',
        start: bool = True,
    ) -> None:
        """Stop the controller, set mode and bitrate and optionally start it again."""
        self._stop()
        self._exec_checked(f'can mode {self._device} {mode}')
        self._exec_checked(f'can bitrate {self._device} {bitrate}')
        self.clear_rx()
        if start:
            self._start()

    def get_state(self) -> CanControllerState:
        """Return controller state, error counters and statistics."""
        lines = self._exec(f'can show {self._device}')
        stats = {}
        for line in lines:
            m = STATS_REGEX.match(line)
            if m:
                stats[m.group('name').strip()] = int(m.group('cnt'))

        return CanControllerState(
            state=self._match_first(lines, STATE_REGEX).group('state'),
            rx_errors=int(self._match_first(lines, RX_ERR_REGEX).group('cnt')),
            tx_errors=int(self._match_first(lines, TX_ERR_REGEX).group('cnt')),
            stats=stats,
        )

    def clear_rx(self) -> None:
        """Drop all received frames not yet consumed."""
        self._rx_queue.clear()
        self._dut.clear_buffer()

    def send(self, msg: Message, timeout: float | None = None) -> None:
        """Send a frame and wait for the TX callback.

        :raises CanShellError: if the frame could not be enqueued or sending failed.
        """
        logger.debug('%s sending: %s', self._device, msg)

        cmd = f'can send {self._device}'
        cmd += ' -e' if msg.is_extended_id else ''
        cmd += ' -r' if msg.is_remote_frame else ''

        if msg.is_extended_id:
            cmd += f' {msg.arbitration_id:08x}'
        else:
            cmd += f' {msg.arbitration_id:03x}'

        if msg.is_remote_frame:
            # The CAN shell derives the DLC of an RTR frame from the number of data arguments
            cmd += ' 00' * msg.dlc
        elif msg.data:
            cmd += ' ' + msg.data.hex(' ', 1)

        lines = self._shell.exec_command(cmd)
        self._collect_frames(lines)
        m = None
        for line in lines:
            m = re.search(r'enqueuing\s+CAN\s+frame\s+#(?P<id>\d+)', line)
            if m:
                break
            fail = re.search(r'failed to enqueue CAN frame #\d+ \(err (?P<err>-?\d+)\)', line)
            if fail:
                err = int(fail.group('err'))
                raise CanShellError(f'frame not enqueued (err {err})', err)

        if m is None:
            raise CanShellError('frame not enqueued')

        frame_num = m.group('id')
        done_regex = (
            r'CAN\s+frame\s+#' + frame_num + r'\s+successfully\s+sent'
            + r'|failed\s+to\s+send\s+CAN\s+frame\s+#' + frame_num + r'\s+\(err\s+(-?\d+)\)'
        )
        lines = self._dut.readlines_until(regex=done_regex, timeout=timeout or DEFAULT_TIMEOUT)
        self._collect_frames(lines)
        for line in lines:
            fail = re.search(r'failed\s+to\s+send\s+CAN\s+frame\s+#' + frame_num
                             + r'\s+\(err\s+(?P<err>-?\d+)\)', line)
            if fail:
                err = int(fail.group('err'))
                raise CanShellError(f'failed to send frame #{frame_num} (err {err})', err)

    def send_nowait(self, msg: Message) -> None:
        """Enqueue a frame without waiting for the TX callback."""
        cmd = f'can send {self._device}'
        cmd += ' -e' if msg.is_extended_id else ''
        cmd += f' {msg.arbitration_id:08x}' if msg.is_extended_id else f' {msg.arbitration_id:03x}'
        if msg.data:
            cmd += ' ' + msg.data.hex(' ', 1)
        self._exec(cmd)

    def wait_tx_result(self, timeout: float) -> int | None:
        """Wait for the result of the last TX enqueued with send_nowait().

        Returns 0 on success, the error code on failure or None on timeout.
        """
        regex = r'successfully\s+sent|failed\s+to\s+send\s+CAN\s+frame'
        try:
            lines = self._dut.readlines_until(regex=regex, timeout=timeout)
        except AssertionError:
            return None
        self._collect_frames(lines)
        for line in lines:
            fail = re.search(r'failed\s+to\s+send\s+CAN\s+frame\s+#\d+\s+\(err\s+(?P<err>-?\d+)\)',
                             line)
            if fail:
                return int(fail.group('err'))
        return 0

    def burst(self, can_id: int, count: int, extended: bool = False,
              timeout: float = 30.0) -> tuple[int, int]:
        """Send count frames from firmware. Returns (sent, failed)."""
        cmd = 'can_host burst'
        cmd += ' -e' if extended else ''
        cmd += f' {can_id:x} {count}'
        lines = self._exec(cmd, timeout=timeout)
        m = self._match_first(lines, BURST_REGEX)
        return int(m.group('sent')), int(m.group('failed'))

    def disable_shell_rx(self) -> None:
        """Remove all CAN shell RX filters, so no frames are printed."""
        self._remove_all_filters()

    def enable_shell_rx(self) -> None:
        """Restore the accept-all CAN shell RX filters."""
        self.set_filters(None)

    def rx_counter_start(self, can_id: int, extended: bool = False) -> None:
        """Start counting frames with the given ID in firmware."""
        cmd = 'can_host rx start'
        cmd += ' -e' if extended else ''
        self._exec_checked(f'{cmd} {can_id:x}')

    def rx_counter_stop(self) -> tuple[int, int, int]:
        """Stop counting frames. Returns (count, gaps, duplicates)."""
        lines = self._exec('can_host rx stop')
        m = self._match_first(lines, RX_COUNT_REGEX)
        return int(m.group('count')), int(m.group('gaps')), int(m.group('dups'))

    def _add_filter(self, can_id: int, can_mask: int, extended: bool) -> None:
        """Add RX filter."""
        cmd = f'can filter add {self._device}'
        cmd += ' -e' if extended else ''

        if extended:
            cmd += f' {can_id:08x} {can_mask:08x}'
        else:
            cmd += f' {can_id:03x} {can_mask:03x}'

        lines = self._exec(cmd)
        m = self._match_first(lines, re.compile(r'filter\s+ID:\s+(?P<id>\d+)'))
        self._filter_ids.append(int(m.group('id')))

    def _remove_filter(self, filter_id: int) -> None:
        """Remove RX filter."""
        if filter_id in self._filter_ids:
            self._filter_ids.remove(filter_id)

        self._exec_checked(f'can filter remove {self._device} {filter_id}')

    def _remove_all_filters(self) -> None:
        """Remove all RX filters."""
        for filter_id in self._filter_ids[:]:
            self._remove_filter(filter_id)

    def _apply_filters(self, filters: CanFilters | None) -> None:
        self._remove_all_filters()

        if filters:
            self._is_filtered = True
        else:
            # Accept all frames if no hardware filters provided
            filters = [
                {'can_id': 0x0, 'can_mask': 0x0},
                {'can_id': 0x0, 'can_mask': 0x0, 'extended': True},
            ]
            self._is_filtered = False

        for can_filter in filters:
            self._add_filter(can_filter['can_id'], can_filter['can_mask'],
                             can_filter.get('extended', False))

    def _recv_internal(self, timeout: float | None) -> tuple[Message | None, bool]:
        if self._rx_queue:
            return self._rx_queue.popleft(), self._is_filtered

        try:
            lines = self._dut.readlines_until(regex=self._frame_regex.pattern,
                                              timeout=timeout or DEFAULT_TIMEOUT)
        except AssertionError:
            return None, self._is_filtered

        self._collect_frames(lines)
        if self._rx_queue:
            return self._rx_queue.popleft(), self._is_filtered
        return None, self._is_filtered

    def recv_all(self, timeout: float) -> list[Message]:
        """Receive frames until no new frame arrives for timeout seconds."""
        frames = []
        while (msg := self.recv(timeout=timeout)) is not None:
            frames.append(msg)
        return frames

    def shutdown(self) -> None:
        if not self._is_shutdown:
            super().shutdown()
            self._stop()
            self._remove_all_filters()
