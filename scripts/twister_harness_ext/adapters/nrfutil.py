# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from __future__ import annotations

import logging
import re
from pathlib import Path
from typing import Literal

from twister_harness_ext.adapters.cmd import run_command

ERASE_MODE = Literal["ERASE_NONE", "ERASE_ALL", "ERASE_CTRL_AP", "ERASE_RANGES_TOUCHED_BY_FIRMWARE"]

logger = logging.getLogger(__name__)


def reset_board(dev_id: str | None = None, reset_kind: str | None = None):
    """Reset a board using nrfutil, optionally specifying a device ID."""
    command = ["nrfutil", "device", "reset"]
    if reset_kind:
        command.extend(["--reset-kind", reset_kind])
    if dev_id:
        command.extend(["--serial-number", dev_id])
    run_command(command)


def erase_board(dev_id: str | None):
    command = ['nrfutil', 'device', 'erase']
    if dev_id:
        command.extend(['--serial-number', dev_id])
    run_command(command)


def flash_with_nrfutil(
    firmware: Path | str,
    dev_id: str,
    erase_mode: ERASE_MODE | None,
    ext_mem_erase_mode: ERASE_MODE | None = None,
):
    command = ['nrfutil', 'device', 'program', '--firmware', str(firmware)]
    if dev_id:
        command.extend(['--serial-number', dev_id])
    options = []
    if erase_mode:
        options += [f"chip_erase_mode={erase_mode}"]
    if ext_mem_erase_mode:
        options += [f"ext_mem_erase_mode={ext_mem_erase_mode}"]
    if options:
        command += ["--options", ",".join(options)]
    run_command(command)


def halt_cpu(dev_id: str | None = None, core: str | None = None) -> None:
    """Halt CPU execution on a device using nrfutil.

    :param dev_id: Optional serial number of the device to target.
    :param core: Optional core selector (e.g., "Application").
    """
    command = ["nrfutil", "device", "halt"]
    if dev_id:
        command.extend(["--serial-number", dev_id])
    if core:
        command.extend(["--core", core])
    run_command(command)


def go_cpu(
    dev_id: str | None = None,
    core: str | None = None,
    program_counter: int | None = None,
    stack_pointer: int | None = None,
) -> None:
    """Resume CPU execution on a device using nrfutil.

    :param dev_id: Optional serial number of the device to target.
    :param core: Optional core selector (e.g., "Application").
    :param program_counter: Optional initial program counter address.
    :param stack_pointer: Optional initial stack pointer address.
    """
    command = ["nrfutil", "device", "go"]
    if program_counter is not None:
        command.extend(["--program-counter", hex(program_counter)])
    if stack_pointer is not None:
        command.extend(["--stack-pointer", hex(stack_pointer)])
    if dev_id:
        command.extend(["--serial-number", dev_id])
    if core:
        command.extend(["--core", core])
    run_command(command)


def nrfutil_write(
    address: str, value: str, dev_id: str | None = None, core: str | None = None
) -> None:
    """Write a value to a specific address on the device.

    :param address: memory address to write to
    :param value: value to write
    :param dev_id: serial number of the device
    :param core: core to target (optional)
    """
    cmd = [
        "nrfutil",
        "device",
        "write",
        "--address",
        address,
        "--value",
        value,
    ]

    if dev_id:
        cmd.extend(["--serial-number", dev_id])
    if core:
        cmd.extend(["--core", core])

    run_command(cmd)


def nrfutil_read(
    address: str | int, bytes_count: int, dev_id: str | None = None, core: str | None = None
) -> str:
    """Read data from a specific address on the device.

    :param address: memory address to read from (can be hex string like "0x1000" or int)
    :param bytes_count: number of bytes to read
    :param dev_id: serial number of the device
    :param core: core to target (optional)
    :return: the data read from the device as a hex string (e.g., "0x12345678")
    """
    cmd = [
        "nrfutil",
        "device",
        "read",
        "--address",
        str(address),
        "--bytes",
        str(bytes_count),
    ]

    if dev_id:
        cmd.extend(["--serial-number", dev_id])
    if core:
        cmd.extend(["--core", core])

    result = run_command(cmd)
    logger.debug(f"nrfutil read output: {result.stdout}")

    # Parse the address from the input to create the regex pattern
    if isinstance(address, str) and address.startswith("0x"):
        addr_int = int(address, 16)
    else:
        addr_int = int(address, 0)  # Handle both hex and decimal  # type: ignore

    # Create regex pattern to match the output format: "0xADDRESS: DATA"
    hex_pattern = rf"0x{addr_int:08X}:\s+(\S+)"
    match = re.search(hex_pattern, result.stdout, re.IGNORECASE)

    if match:
        hex_data = f"0x{match.group(1).lower()}"
        logger.info(f"Parsed hex data: {hex_data}")
        return hex_data
    else:
        raise ValueError(f"Could not parse hex data from nrfutil output: {result.stdout}")
