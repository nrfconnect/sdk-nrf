# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
from __future__ import annotations

import logging
import os
import shlex
import subprocess
from pathlib import Path
from typing import Literal

ERASE_MODE = Literal["ERASE_NONE", "ERASE_ALL", "ERASE_CTRL_AP", "ERASE_RANGES_TOUCHED_BY_FIRMWARE"]

logger = logging.getLogger(__name__)


def normalize_path(path: str | None) -> str | None:
    if path is not None:
        path = os.path.expanduser(os.path.expandvars(path))
        path = os.path.normpath(os.path.abspath(path))
    return path


def run_command(command: list[str], timeout: int = 30) -> subprocess.CompletedProcess:
    logger.info(f"CMD: {shlex.join(command)}")
    ret: subprocess.CompletedProcess = subprocess.run(
        command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout
    )
    if ret.returncode:
        logger.error(f"Failed command: {shlex.join(command)}")
        logger.info(ret.stdout)
        raise subprocess.CalledProcessError(ret.returncode, command)
    return ret


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


def flash_with_nrfutil(firmware: Path | str, dev_id: str, erase_mode: ERASE_MODE | None):
    command = ['nrfutil', 'device', 'program', '--firmware', str(firmware)]
    if dev_id:
        command.extend(['--serial-number', dev_id])
    options = []
    if erase_mode:
        options += [f"chip_erase_mode={erase_mode}"]
    if options:
        command += ["--options"] + options
    run_command(command)


def flash_board(build_dir: Path | str, dev_id: str | None, erase: bool = False):
    command = ['west', 'flash', '--skip-rebuild', '-d', str(build_dir)]
    if dev_id:
        command.extend(['--dev-id', dev_id])
    if erase:
        command.extend(['--erase'])
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
