#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""H4 transport: send HCI commands and read events on a serial port."""

import struct
import sys

import serial

from utp.hci_events import (
    CommandCompleteEvent,
    CommandStatusEvent,
    HciEvent,
    LeMetaEvent,
)

# H4 packet indicators
H4_CMD = 0x01
H4_EVT = 0x04

# HCI commands
OPCODE_RESET = 0x0C03
OPCODE_SET_EVENT_MASK = 0x0C01
OPCODE_LE_SET_EVENT_MASK = 0x2001


def read_exact(port: serial.Serial, n: int) -> bytes:
    data = port.read(n)
    if len(data) != n:
        sys.exit("Timeout waiting for data from the device")
    return data


def read_event(port: serial.Serial) -> HciEvent:
    """Read one H4 event and return it as an HciEvent instance."""
    while read_exact(port, 1)[0] != H4_EVT:
        pass
    code, length = read_exact(port, 2)
    params = read_exact(port, length)
    return HciEvent.parse(code, params)


def send_command(port: serial.Serial, opcode: int, params: bytes = b"") -> None:
    packet = bytes([H4_CMD]) + struct.pack("<HB", opcode, len(params)) + params
    port.write(packet)


def hci_command(
    port: serial.Serial, opcode: int, params: bytes = b"", completion_subevent: int | None = None
) -> tuple[int, bytes | None]:
    """Send an HCI command and wait for its Command Complete or Command Status.

    If completion_subevent is given and the command succeeded, also wait for that LE meta
    subevent. Return (status, parameters of the subevent or None).
    """
    send_command(port, opcode, params)
    while True:
        event = read_event(port)
        if isinstance(event, (CommandCompleteEvent, CommandStatusEvent)) and event.opcode == opcode:
            break
    if event.status != 0 or completion_subevent is None:
        return event.status, None
    while True:
        event = read_event(port)
        if isinstance(event, LeMetaEvent) and event.subevent == completion_subevent:
            return 0, event.params


def hci_reset(port: serial.Serial) -> None:
    steps = (
        ("HCI Reset", OPCODE_RESET, b""),
        # LE Meta events (which carry LE UTP Receive) are disabled by default. Enable them,
        # and all LE subevents.
        ("Set Event Mask", OPCODE_SET_EVENT_MASK, b"\xff" * 8),
        ("LE Set Event Mask", OPCODE_LE_SET_EVENT_MASK, b"\xff" * 8),
    )
    for name, opcode, params in steps:
        print(f"{name}...", end=" ", flush=True)
        status, _ = hci_command(port, opcode, params)
        if status != 0:
            sys.exit(f"{name} failed: status 0x{status:02x}")
        print("ok")
