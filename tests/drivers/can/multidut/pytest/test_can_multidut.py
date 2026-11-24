# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Multi-DUT tests of CAN communication between a DK with a hardware M_CAN controller, a DK
with a software sCAN controller and a host interface.

The host interface is up in listen-only mode at HOST_BITRATE for the whole session. It never
ACKs nor transmits, so every frame must be acknowledged by the other DK, or by the M_CAN
itself in external loopback mode (which drives the bus and ignores the missing
ACK). At HOST_BITRATE the host verifies the bus traffic as a sniffer. At other bitrates it
does not take part in the communication at all.
"""

import logging
import threading
import time

import pytest
from can import Message
from can_shell import CanShellBus, CanShellError
from conftest import (DEFAULT_BITRATES, HOST_BITRATE, SETTLE_ID, HostCanInfo, Sniffer,
                      read_host_can_info)

logger = logging.getLogger(__name__)

TIMEOUT = 2.0
NO_FRAME_TIMEOUT = 0.5
BURST_COUNT = 200
BURST_ID_MCAN = 0x100
BURST_ID_SCAN = 0x101
RECOVERY_ROUNDS = 3
RECOVERY_BURST = 130
TX_ATTEMPT_ERRORS = ('bit errors', 'stuff errors', 'crc errors', 'form errors', 'ack errors')
NO_ACK_BITRATE = 500000
MISMATCH_BITRATES = (500000, 250000)
SCAN_NO_UNACKED_RX = 'sCAN in listen-only mode does not deliver frames that nobody ACKed'
SCAN_NO_FAULT_CONFINEMENT = 'sCAN does not increment TEC on ACK errors and stays error active'


def _data_frames() -> list[pytest.param]:
    frames = [
        pytest.param(Message(arbitration_id=0x000, is_extended_id=False), id='std_0x000_dlc0'),
        pytest.param(Message(arbitration_id=0x7FF, is_extended_id=False, data=[0xFF] * 8),
                     id='std_0x7ff_ff'),
        pytest.param(Message(arbitration_id=0x123, is_extended_id=False, data=[0x55] * 8),
                     id='std_55'),
        pytest.param(Message(arbitration_id=0x2AA, is_extended_id=False, data=[0xAA] * 8),
                     id='std_aa'),
        pytest.param(Message(arbitration_id=0x100, is_extended_id=False, data=[0x00] * 8),
                     id='std_stuffing_00'),
        pytest.param(Message(arbitration_id=0x0F0, is_extended_id=False,
                             data=[0x0F, 0xF0, 0x0F, 0xF0, 0x1F, 0xE0, 0x3E, 0x7C]),
                     id='std_stuffing_runs'),
        pytest.param(Message(arbitration_id=0x00000000, is_extended_id=True, data=[0x01]),
                     id='ext_0x0'),
        pytest.param(Message(arbitration_id=0x1FFFFFFF, is_extended_id=True, data=[0xFF] * 8),
                     id='ext_0x1fffffff_ff'),
        pytest.param(Message(arbitration_id=0x15555555, is_extended_id=True,
                             data=[0x0F, 0xF0, 0x00, 0xFF]),
                     id='ext_0x15555555'),
        pytest.param(Message(arbitration_id=0x321, is_extended_id=False, is_remote_frame=True,
                             dlc=4),
                     id='std_rtr_dlc4'),
        pytest.param(Message(arbitration_id=0x0ABCDEF, is_extended_id=True,
                             is_remote_frame=True, dlc=8),
                     id='ext_rtr_dlc8'),
    ]
    frames += [
        pytest.param(Message(arbitration_id=0x200 + n, is_extended_id=False,
                             data=list(range(n))), id=f'std_dlc{n}')
        for n in range(9)
    ]
    return frames


DATA_FRAMES = [p.values[0] for p in _data_frames()]


def check_rx(tx: Message, rx: Message | None) -> None:
    """Check if received message matches transmitted message."""
    __tracebackhide__ = True  # pylint: disable=unused-variable

    if rx is None:
        pytest.fail(f'no message received for "{tx}"')

    if not tx.equals(rx, timestamp_delta=None, check_channel=False, check_direction=False):
        pytest.fail(f'rx message "{rx}" not equal to tx message "{tx}"')


def check_sequence(tx: list[Message], rx: list[Message], who: str) -> None:
    """Check that rx contains exactly the frames in tx, in order."""
    __tracebackhide__ = True  # pylint: disable=unused-variable

    rx = [m for m in rx if m.arbitration_id != SETTLE_ID]
    if len(rx) != len(tx):
        pytest.fail(f'{who}: expected {len(tx)} frames, got {len(rx)}: {rx}')
    for t, r in zip(tx, rx, strict=True):
        check_rx(t, r)


def check_error_active(*dks: CanShellBus) -> None:
    """Check that the DK controllers are error active."""
    __tracebackhide__ = True  # pylint: disable=unused-variable

    for dk in dks:
        state = dk.get_state()
        logger.info('%s: %s', dk.name, state)
        if state.state != 'error-active':
            pytest.fail(f'{dk.name} is {state.state} (rx {state.rx_errors}, '
                        f'tx {state.tx_errors}, stats {state.stats})')


def recover_error_counters(*dks: CanShellBus) -> None:
    """Bring the DK controllers back to error active with successful traffic.

    Not all controllers reset their error counters on stop/start (M_CAN keeps them), so
    errors provoked by earlier tests would leak into later ones. Every successfully
    transmitted frame decrements TEC and every received one decrements REC.
    """
    __tracebackhide__ = True  # pylint: disable=unused-variable

    for _ in range(RECOVERY_ROUNDS):
        states = {dk: dk.get_state() for dk in dks}
        if all(s.state == 'error-active' and s.tx_errors == 0 and s.rx_errors == 0
               for s in states.values()):
            return
        logger.info('recovering error counters: %s',
                    {dk.name: (s.state, s.tx_errors, s.rx_errors) for dk, s in states.items()})
        for dk in dks:
            dk.disable_shell_rx()
        failed = {dk: dk.burst(SETTLE_ID, RECOVERY_BURST)[1] for dk in dks}
        for dk in dks:
            if failed[dk]:
                # Abort a pending TX first: sCAN blocks configuration calls while a frame is
                # being retransmitted without ACK
                dk.stop()
                dk.enable_shell_rx()
                dk.start()
            else:
                dk.enable_shell_rx()
            dk.clear_rx()

    check_error_active(*dks)


@pytest.fixture(name='prepare')
def fixture_prepare(dk_mcan: CanShellBus, dk_scan: CanShellBus):
    """Return a callable configuring both DKs to a common bitrate."""

    def prepare(bitrate: int, modes: dict[CanShellBus, str] | None = None) -> None:
        modes = modes or {}
        for dk in (dk_mcan, dk_scan):
            mode = modes.get(dk, 'normal')
            if bitrate > dk.max_bitrate:
                pytest.skip(f'{dk.name} max bitrate {dk.max_bitrate}')
            try:
                dk.configure(bitrate, mode=mode)
            except CanShellError as exc:
                pytest.skip(f'{dk.name} does not support {bitrate} bps in mode {mode}: {exc}')

        if not modes:
            recover_error_counters(dk_mcan, dk_scan)

    yield prepare

    for dk in (dk_mcan, dk_scan):
        dk.stop()
        dk.enable_shell_rx()


def _sniffer_at(request, bitrate: int) -> Sniffer | None:
    """Return the host sniffer if it can see traffic at bitrate, None otherwise."""
    return request.getfixturevalue('sniffer') if bitrate == HOST_BITRATE else None


@pytest.fixture(name='directions', params=['mcan_to_scan', 'scan_to_mcan'])
def fixture_directions(request, dk_mcan: CanShellBus,
                       dk_scan: CanShellBus) -> tuple[CanShellBus, CanShellBus]:
    """Return (sender, receiver) for both directions."""
    if request.param == 'mcan_to_scan':
        return dk_mcan, dk_scan
    return dk_scan, dk_mcan


class TestBidirectional:
    """Scenarios 1 and 2: frame exchange between DKs at every bitrate."""

    def test_exchange(self, request, prepare, directions, bitrate: int,
                      host_info: HostCanInfo | None) -> None:
        """Send all test frames from one DK to the other and verify them.

        At the host bitrate the sniffer additionally verifies that every frame appeared on
        the bus exactly once, in order, without error frames.
        """
        sender, receiver = directions
        sniffer = _sniffer_at(request, bitrate)
        prepare(bitrate)
        host_before = None
        if sniffer:
            sniffer.start()
            host_before = read_host_can_info(host_info.channel)

        for msg in DATA_FRAMES:
            sender.send(msg, timeout=TIMEOUT)
            check_rx(msg, receiver.recv(timeout=TIMEOUT))

        extra = receiver.recv_all(timeout=NO_FRAME_TIMEOUT)
        assert not extra, f'unexpected frames on {receiver.name}: {extra}'
        check_error_active(sender, receiver)

        if sniffer:
            sniffer.wait_for(len(DATA_FRAMES), TIMEOUT)
            sniffer.stop()
            check_sequence(DATA_FRAMES, sniffer.frames, 'sniffer')
            assert not sniffer.errors, f'error frames on bus: {sniffer.errors}'
            host_after = read_host_can_info(host_info.channel)
            assert host_after.bus_errors == host_before.bus_errors, \
                f'host bus errors {host_before.bus_errors} -> {host_after.bus_errors}'


class TestHostSniffer:
    """Scenario 3: host as a passive observer at its own bitrate."""

    @pytest.mark.parametrize('msg', _data_frames())
    def test_dk_to_all(self, prepare, directions, sniffer: Sniffer, msg: Message) -> None:
        """A frame sent by a DK is received by the other DK and seen by the host."""
        sender, receiver = directions
        prepare(HOST_BITRATE)
        sniffer.start()

        sender.send(msg, timeout=TIMEOUT)
        check_rx(msg, receiver.recv(timeout=TIMEOUT))
        frames = sniffer.wait_for(1, TIMEOUT)
        sniffer.stop()
        check_sequence([msg], frames, 'host')


class TestArbitration:
    """Scenario 4: concurrent bursts from both DKs."""

    def test_concurrent_bursts(self, prepare, dk_mcan: CanShellBus, dk_scan: CanShellBus,
                               sniffer: Sniffer) -> None:
        """Both DKs send bursts with adjacent IDs at the same time.

        Arbitration happens on every frame collision. Checks: no losses, no duplicates,
        per-sender order preserved (also on the bus), controllers error active afterwards.
        """
        prepare(HOST_BITRATE)
        for dk in (dk_mcan, dk_scan):
            dk.disable_shell_rx()
        dk_mcan.rx_counter_start(BURST_ID_SCAN)
        dk_scan.rx_counter_start(BURST_ID_MCAN)
        sniffer.start()

        results = {}

        def run(dk: CanShellBus, can_id: int) -> None:
            results[dk.name] = dk.burst(can_id, BURST_COUNT, timeout=60.0)

        threads = [threading.Thread(target=run, args=(dk_mcan, BURST_ID_MCAN)),
                   threading.Thread(target=run, args=(dk_scan, BURST_ID_SCAN))]
        for t in threads:
            t.start()
        for t in threads:
            t.join()

        time.sleep(0.2)
        mcan_rx = dk_mcan.rx_counter_stop()
        scan_rx = dk_scan.rx_counter_stop()
        sniffer.stop()
        for dk in (dk_mcan, dk_scan):
            logger.info('%s after bursts: %s', dk.name, dk.get_state())
        logger.info('burst results %s, mcan rx %s, scan rx %s', results, mcan_rx, scan_rx)

        assert results[dk_mcan.name] == (BURST_COUNT, 0), f'mcan burst: {results[dk_mcan.name]}'
        assert results[dk_scan.name] == (BURST_COUNT, 0), f'scan burst: {results[dk_scan.name]}'
        for name, (count, gaps, dups) in ((dk_mcan.name, mcan_rx), (dk_scan.name, scan_rx)):
            assert gaps == 0 and count - dups == BURST_COUNT, \
                (f'{name} received {count - dups} of {BURST_COUNT} frames '
                 f'({gaps} gaps, {dups} duplicates)')
            assert dups == 0, f'{name} received {dups} duplicates on an error free bus'
        check_error_active(dk_mcan, dk_scan)

        for can_id in (BURST_ID_MCAN, BURST_ID_SCAN):
            seqs = [int.from_bytes(m.data[:4], 'big') for m in sniffer.frames
                    if m.arbitration_id == can_id]
            assert seqs == list(range(BURST_COUNT)), \
                f'sniffer sequence for 0x{can_id:x} broken ({len(seqs)} frames)'
        assert not sniffer.errors, f'error frames on bus: {sniffer.errors}'


FILTERS = [
    {'can_id': 0x120, 'can_mask': 0x7F0},
    {'can_id': 0x01234500, 'can_mask': 0x1FFFFF00, 'extended': True},
]
FILTER_FRAMES = [
    (Message(arbitration_id=0x120, is_extended_id=False, data=[1]), True),
    (Message(arbitration_id=0x12F, is_extended_id=False, data=[2]), True),
    (Message(arbitration_id=0x130, is_extended_id=False, data=[3]), False),
    (Message(arbitration_id=0x020, is_extended_id=False, data=[4]), False),
    (Message(arbitration_id=0x01234501, is_extended_id=True, data=[5]), True),
    (Message(arbitration_id=0x012345FF, is_extended_id=True, data=[6]), True),
    (Message(arbitration_id=0x01234601, is_extended_id=True, data=[7]), False),
    (Message(arbitration_id=0x00000120, is_extended_id=True, data=[8]), False),
]


class TestFilters:
    """Scenario 5: RX filters at the lowest and the highest bitrate."""

    @pytest.mark.parametrize('filter_bitrate', [20000, 1000000], ids=['20k', '1M'])
    def test_rx_filters(self, request, prepare, directions, filter_bitrate: int) -> None:
        """Only frames matching the receiver filters are delivered."""
        sender, receiver = directions
        sniffer = _sniffer_at(request, filter_bitrate)
        prepare(filter_bitrate)
        receiver.set_filters(FILTERS)
        if sniffer:
            sniffer.start()

        for msg, _ in FILTER_FRAMES:
            sender.send(msg, timeout=TIMEOUT)

        expected = [msg for msg, match in FILTER_FRAMES if match]
        received = receiver.recv_all(timeout=NO_FRAME_TIMEOUT)
        receiver.enable_shell_rx()
        check_sequence(expected, received, receiver.name)

        if sniffer:
            sniffer.stop()
            check_sequence([msg for msg, _ in FILTER_FRAMES], sniffer.frames, 'sniffer')


class TestListenOnly:
    """Scenario 6: listen-only mode."""

    @pytest.mark.xfail(reason=SCAN_NO_UNACKED_RX)
    def test_listen_only_receives(self, prepare, dk_mcan: CanShellBus, dk_scan: CanShellBus,
                                  sniffer: Sniffer) -> None:
        """A listen-only DK receives frames nobody ACKs and does not transmit.

        With the host listen-only too, a frame can only complete without ACK when the sender
        ignores the missing ACK: M_CAN does in external loopback mode, while still driving the
        bus. sCAN loopback is internal only, so the listener is always the sCAN.
        """
        sender, listener = dk_mcan, dk_scan
        if 'loopback' not in sender.capabilities:
            pytest.skip(f'{sender.name} does not support loopback')
        prepare(HOST_BITRATE, modes={sender: 'loopback', listener: 'listen-only'})
        sniffer.start()

        frames = DATA_FRAMES[:6]
        for msg in frames:
            sender.send(msg, timeout=TIMEOUT)
            check_rx(msg, listener.recv(timeout=TIMEOUT))

        probe = Message(arbitration_id=0x555, is_extended_id=False, data=[0xAB])
        try:
            listener.send(probe, timeout=NO_FRAME_TIMEOUT)
            sent = True
        except (CanShellError, AssertionError):
            sent = False
        sniffer.stop(linger=NO_FRAME_TIMEOUT)
        probes = [m for m in sniffer.frames if m.arbitration_id == probe.arbitration_id]
        assert not probes, \
            f'{listener.name} transmitted in listen-only mode (send returned {sent})'
        check_sequence(frames, sniffer.frames, 'sniffer')

    def test_no_ack_from_listen_only(self, prepare, directions, dk_scan: CanShellBus) -> None:
        """With the only other receiver in listen-only mode, frames are not acknowledged."""
        sender, listener = directions
        bitrate = NO_ACK_BITRATE
        prepare(bitrate)
        listener.configure(bitrate, mode='listen-only')

        msg = Message(arbitration_id=0x456, is_extended_id=False, data=[0x11, 0x22])
        sender.send_nowait(msg)
        result = sender.wait_tx_result(timeout=1.0)
        assert result != 0, 'frame acknowledged although no node is allowed to ACK'

        state = sender.get_state()
        logger.info('%s after unacknowledged TX: %s', sender.name, state)
        assert state.stats.get('ack errors', 1) > 0, f'{sender.name} reported no ACK errors'

        sender.stop()
        sender.clear_rx()
        listener.configure(bitrate, mode='normal')
        sender.start()
        sender.send(msg, timeout=TIMEOUT)
        check_rx(msg, listener.recv(timeout=TIMEOUT))

        if state.state not in ('error-warning', 'error-passive'):
            if sender is dk_scan:
                pytest.xfail(SCAN_NO_FAULT_CONFINEMENT)
            pytest.fail(f'{sender.name} is {state.state} with TEC {state.tx_errors} after '
                        f'{state.stats.get("ack errors")} ACK errors; ISO 11898-1 requires '
                        f'TEC += 8 per ACK error until error passive')


class TestOneShot:
    """Scenario 7: one-shot mode."""

    def test_one_shot_success(self, prepare, directions, sniffer: Sniffer) -> None:
        """An acknowledged one-shot frame appears on the bus exactly once."""
        sender, receiver = directions
        if 'one-shot' not in sender.capabilities:
            pytest.skip(f'{sender.name} does not support one-shot')
        prepare(HOST_BITRATE)
        sender.configure(HOST_BITRATE, mode='one-shot')
        sniffer.start()

        msg = Message(arbitration_id=0x0A5, is_extended_id=False, data=[1, 2, 3])
        sender.send(msg, timeout=TIMEOUT)
        check_rx(msg, receiver.recv(timeout=TIMEOUT))
        sniffer.stop(linger=0.3)
        check_sequence([msg], sniffer.frames, 'sniffer')

    def test_one_shot_no_ack(self, prepare, directions) -> None:
        """Without ACK a one-shot frame fails after a single attempt."""
        sender, receiver = directions
        if 'one-shot' not in sender.capabilities:
            pytest.skip(f'{sender.name} does not support one-shot')
        bitrate = NO_ACK_BITRATE
        prepare(bitrate)
        receiver.stop()
        sender.configure(bitrate, mode='one-shot')

        msg = Message(arbitration_id=0x0A6, is_extended_id=False, data=[4, 5, 6])
        sender.send_nowait(msg)
        result = sender.wait_tx_result(timeout=TIMEOUT)
        assert result is not None, 'one-shot TX did not complete'
        assert result != 0, 'one-shot TX reported success without ACK'

        state = sender.get_state()
        logger.info('%s after one-shot without ACK: %s', sender.name, state)
        attempts = sum(state.stats.get(k, 0) for k in TX_ATTEMPT_ERRORS)
        assert attempts == 1, f'expected a single failed attempt, got {state.stats}'


class TestBitrateMismatch:
    """Scenario 8: mismatched bitrates and recovery."""

    def test_mismatch_and_recovery(self, prepare, dk_mcan: CanShellBus,
                                   dk_scan: CanShellBus) -> None:
        """No frames pass between DKs at different bitrates; communication recovers."""
        rate_mcan, rate_scan = MISMATCH_BITRATES

        dk_mcan.configure(rate_mcan)
        dk_scan.configure(rate_scan)

        msg = Message(arbitration_id=0x3C3, is_extended_id=False, data=[0xC3] * 8)
        dk_mcan.send_nowait(msg)
        assert dk_scan.recv(timeout=1.0) is None, 'frame received at mismatched bitrate'
        assert dk_mcan.wait_tx_result(timeout=NO_FRAME_TIMEOUT) != 0, \
            'frame acknowledged at mismatched bitrate'

        state = dk_scan.get_state()
        logger.info('%s at mismatched bitrate: %s', dk_scan.name, state)
        assert state.rx_errors > 0 or any(state.stats.values()), \
            f'{dk_scan.name} detected no errors at mismatched bitrate'

        prepare(rate_scan)
        dk_mcan.send(msg, timeout=TIMEOUT)
        check_rx(msg, dk_scan.recv(timeout=TIMEOUT))
        dk_scan.send(msg, timeout=TIMEOUT)
        check_rx(msg, dk_mcan.recv(timeout=TIMEOUT))
        check_error_active(dk_mcan, dk_scan)


class TestBitrateSequence:
    """Scenario 9: walk through all bitrates in one session."""

    def test_bitrate_sequence(self, request, prepare, dk_mcan: CanShellBus,
                              dk_scan: CanShellBus) -> None:
        """Reconfigure the bitrate repeatedly without stale frames between steps."""
        opt = request.config.getoption('--can-bitrates')
        bitrates = [int(b) for b in opt.split(',')] if opt else DEFAULT_BITRATES

        for idx, bitrate in enumerate(bitrates + list(reversed(bitrates))):
            prepare(bitrate)
            msg = Message(arbitration_id=0x600 + idx, is_extended_id=False,
                          data=list(bitrate.to_bytes(4, 'big')))
            for sender, receiver in ((dk_mcan, dk_scan), (dk_scan, dk_mcan)):
                sender.send(msg, timeout=TIMEOUT)
                received = receiver.recv_all(timeout=NO_FRAME_TIMEOUT)
                check_sequence([msg], received, f'{receiver.name} at {bitrate}')
