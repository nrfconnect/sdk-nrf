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


def run_command(command: list[str], timeout: int = 30):
    logger.info(f"CMD: {shlex.join(command)}")
    ret: subprocess.CompletedProcess = subprocess.run(
        command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout
    )
    if ret.returncode:
        logger.error(f"Failed command: {shlex.join(command)}")
        logger.info(ret.stdout)
        raise subprocess.CalledProcessError(ret.returncode, command)


def reset_board(dev_id: str | None = None):
    command = ['nrfutil', 'device', 'reset']
    if dev_id:
        command.extend(['--serial-number', dev_id])
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
