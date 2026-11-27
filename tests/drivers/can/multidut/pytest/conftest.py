# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Configuration of the CAN multi-DUT test suite.

Participants on a shared CAN bus:
  - DK with a hardware M_CAN controller (classic CAN, FD capable), fixture 'mcan'
  - DK with a software sCAN controller (classic CAN only), fixture 'scan'
  - host SocketCAN interface can0, set up in listen-only mode at HOST_BITRATE before
    the session and left unchanged during it. It never ACKs, transmits nor sends
    error flags, so it is only a sniffer at HOST_BITRATE and invisible at other bitrates.
"""

import json
import logging
import re
import subprocess
import threading
import time
from dataclasses import dataclass, field

import pytest
from can import Bus, BusABC, Message
from can_shell import CanShellBus
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)

CONTROLLER_MCAN = 'mcan'
CONTROLLER_SCAN = 'scan'
CONTROLLERS = (CONTROLLER_MCAN, CONTROLLER_SCAN)

DEFAULT_BITRATES = [20000, 50000, 100000, 125000, 250000, 500000, 800000, 1000000]
SLOW_BITRATES = [10000]

HOST_CHANNEL = 'can0'
HOST_BITRATE = 1000000
SETTLE_ID = 0x7F0

# SocketCAN error classes (linux/can/error.h) indicating disturbed frames. Controller
# state notifications (CAN_ERR_CRTL) and restarts are not bus errors.
CAN_ERR_PROT = 0x08
CAN_ERR_TRX = 0x10
CAN_ERR_ACK = 0x20
CAN_ERR_BUSERROR = 0x80
BUS_ERROR_MASK = CAN_ERR_PROT | CAN_ERR_TRX | CAN_ERR_ACK | CAN_ERR_BUSERROR


def pytest_addoption(parser) -> None:
    """Add local parser options to pytest."""
    parser.addoption(
        '--can-bitrates',
        default=None,
        help='Comma separated list of nominal bitrates to test (default: built-in list)',
    )
    parser.addoption(
        '--can-slow',
        action='store_true',
        default=False,
        help='Also test bitrates below 20 kbit/s',
    )


def pytest_generate_tests(metafunc) -> None:
    if 'bitrate' in metafunc.fixturenames:
        opt = metafunc.config.getoption('--can-bitrates')
        if opt:
            bitrates = [int(b) for b in opt.split(',')]
        else:
            bitrates = list(DEFAULT_BITRATES)
            if metafunc.config.getoption('--can-slow'):
                bitrates = SLOW_BITRATES + bitrates
        metafunc.parametrize('bitrate', bitrates, ids=[f'{b // 1000}k' for b in bitrates])


@dataclass
class HostCanInfo:
    """Host SocketCAN interface configuration and state from 'ip -json'."""

    channel: str
    bitrate: int
    listen_only: bool
    state: str
    rx_errors: int
    tx_errors: int
    bus_errors: int
    stats: dict = field(default_factory=dict)


def read_host_can_info(channel: str) -> HostCanInfo:
    """Read host CAN interface configuration without requiring root privileges."""
    out = subprocess.run(
        ['ip', '-details', '-statistics', '-json', 'link', 'show', channel],
        check=True, capture_output=True, text=True,
    ).stdout
    link = json.loads(out)[0]
    info = link['linkinfo']['info_data']
    ctrlmode = info.get('ctrlmode', [])

    return HostCanInfo(
        channel=channel,
        bitrate=info.get('bittiming', {}).get('bitrate', 0),
        listen_only='LISTEN-ONLY' in ctrlmode,
        state=info.get('state', 'UNKNOWN'),
        rx_errors=info.get('berr_counter', {}).get('rx', 0),
        tx_errors=info.get('berr_counter', {}).get('tx', 0),
        bus_errors=link['linkinfo'].get('info_xstats', {}).get('bus_error', 0),
        stats=link.get('stats64', {}),
    )


def host_config_problems(info: HostCanInfo) -> list[str]:
    """Return deviations of the host interface from the expected session configuration."""
    problems = []
    if info.state == 'STOPPED':
        problems.append('interface is down')
    if not info.listen_only:
        problems.append('not in listen-only mode')
    if info.bitrate != HOST_BITRATE:
        problems.append(f'bitrate is {info.bitrate} instead of {HOST_BITRATE}')
    return problems


class Sniffer:
    """Collects all traffic seen by the host interface in a background thread."""

    def __init__(self, bus: BusABC) -> None:
        self._bus = bus
        self._frames: list[Message] = []
        self._errors: list[Message] = []
        self._lock = threading.Lock()
        self._running = threading.Event()
        self._thread: threading.Thread | None = None

    def start(self) -> None:
        while self._bus.recv(timeout=0) is not None:
            pass
        with self._lock:
            self._frames.clear()
            self._errors.clear()
        self._running.set()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def _run(self) -> None:
        while self._running.is_set():
            msg = self._bus.recv(timeout=0.05)
            if msg is None:
                continue
            with self._lock:
                if msg.is_error_frame:
                    if msg.arbitration_id & BUS_ERROR_MASK:
                        self._errors.append(msg)
                    else:
                        logger.info('host controller notification: %s', msg)
                else:
                    self._frames.append(msg)

    def stop(self, linger: float = 0.2) -> None:
        if self._thread is None:
            return
        time.sleep(linger)
        self._running.clear()
        if self._thread:
            self._thread.join()
            self._thread = None

    @property
    def frames(self) -> list[Message]:
        with self._lock:
            return list(self._frames)

    @property
    def errors(self) -> list[Message]:
        with self._lock:
            return list(self._errors)

    def wait_for(self, count: int, timeout: float) -> list[Message]:
        """Wait until at least count frames were captured."""
        deadline = time.time() + timeout
        while time.time() < deadline:
            if len(self.frames) >= count:
                break
            time.sleep(0.02)
        return self.frames


def _chosen(shell: Shell) -> tuple[str, str]:
    """Return the zephyr,canbus device name and its controller type reported by firmware."""
    chosen_regex = re.compile(r'zephyr,canbus:\s+(\S+)')
    controller_regex = re.compile(r'controller:\s+(\S+)')
    chosen = controller = None
    for line in shell.get_filtered_output(shell.exec_command('can_host chosen')):
        if m := chosen_regex.search(line):
            chosen = m.group(1)
        elif m := controller_regex.search(line):
            controller = m.group(1)
    if chosen is None or controller is None:
        pytest.fail('zephyr,canbus chosen device not found or not ready')
    return chosen, controller


def _fixture_controllers(dut: DeviceAdapter) -> set[str]:
    return {f.split(sep=':', maxsplit=1)[0] for f in dut.device_config.fixtures or []} \
        & set(CONTROLLERS)


@pytest.fixture(name='dk_buses', scope='session')
def fixture_dk_buses(duts: list[DeviceAdapter], shells: list[Shell]) -> dict[str, CanShellBus]:
    """Return CAN shell buses of both DKs, keyed by the controller type."""
    buses = {}
    for dut, shell in zip(duts, shells, strict=True):
        chosen, controller = _chosen(shell)
        logger.info('DUT %s: zephyr,canbus is "%s" (%s)', dut.device_config.id, chosen,
                    controller)
        if controller not in CONTROLLERS:
            pytest.fail(f'DUT {dut.device_config.id}: unsupported controller "{controller}"')
        if (fixtures := _fixture_controllers(dut)) and controller not in fixtures:
            pytest.fail(f'DUT {dut.device_config.id}: firmware reports "{controller}" '
                        f'controller, but the DUT has fixture {sorted(fixtures)}')
        if controller in buses:
            pytest.fail(f'more than one DUT with "{controller}" controller')
        buses[controller] = CanShellBus(dut, shell, chosen)

    missing = [c for c in CONTROLLERS if c not in buses]
    if missing:
        for bus in buses.values():
            bus.shutdown()
        pytest.fail(f'this test requires DUTs with {" and ".join(CONTROLLERS)} controllers, '
                    f'missing: {", ".join(missing)}')

    yield buses

    for bus in buses.values():
        bus.shutdown()


@pytest.fixture(name='dk_mcan', scope='session')
def fixture_dk_mcan(dk_buses: dict[str, CanShellBus]) -> CanShellBus:
    """CAN bus of the DK with the hardware M_CAN controller."""
    return dk_buses[CONTROLLER_MCAN]


@pytest.fixture(name='dk_scan', scope='session')
def fixture_dk_scan(dk_buses: dict[str, CanShellBus]) -> CanShellBus:
    """CAN bus of the DK with the software sCAN controller."""
    return dk_buses[CONTROLLER_SCAN]


@pytest.fixture(name='host_info', scope='session', autouse=True)
def fixture_host_info() -> HostCanInfo | None:
    """Return the host interface configuration, checked once at session start.

    A host deviating from the expected configuration only fails the tests using it, but
    it may also disturb the others (e.g. by ACKing frames), hence the warning.
    """
    try:
        info = read_host_can_info(HOST_CHANNEL)
    except (OSError, subprocess.CalledProcessError, KeyError, IndexError, ValueError) as exc:
        logger.warning('cannot read host %s configuration: %s', HOST_CHANNEL, exc)
        return None

    logger.info('host %s: bitrate %d, listen-only %s, state %s',
                info.channel, info.bitrate, info.listen_only, info.state)
    problems = host_config_problems(info)
    if problems:
        logger.warning('host %s is not configured as expected (%s), tests using it will fail '
                       'and other tests may be disturbed', HOST_CHANNEL, ', '.join(problems))
    return info


@pytest.fixture(name='host_bus', scope='session')
def fixture_host_bus(host_info: HostCanInfo | None) -> BusABC:
    """Return the host CAN bus, failing the requesting tests if the host is not usable."""
    expected = f'up in listen-only mode at {HOST_BITRATE} bps before the session'
    if host_info is None:
        pytest.fail(f'this test needs the host CAN interface {HOST_CHANNEL} {expected}, '
                    f'but its configuration cannot be read')
    problems = host_config_problems(host_info)
    if problems:
        pytest.fail(f'this test needs the host CAN interface {HOST_CHANNEL} {expected}: '
                    f'{", ".join(problems)}')

    bus = Bus(interface='socketcan', channel=HOST_CHANNEL)
    yield bus
    bus.shutdown()


@pytest.fixture
def sniffer(host_bus: BusABC) -> Sniffer:
    """Capture host bus traffic during a test, after start() is called."""
    snf = Sniffer(host_bus)
    yield snf
    snf.stop(linger=0)
