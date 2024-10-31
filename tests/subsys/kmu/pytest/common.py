# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
from __future__ import annotations

import logging
import shlex
import subprocess

from pathlib import Path

logger = logging.getLogger(__name__)

APP_KEYS_FOR_KMU = Path(__file__).resolve().parent.parent / 'keys'


def run_command(command: list[str], timeout: int = 30):
    logger.info(f"CMD: {shlex.join(command)}")
    ret: subprocess.CompletedProcess = subprocess.run(
        command, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, timeout=timeout)
    if ret.returncode:
        logger.error(f"Failed command: {shlex.join(command)}")
        logger.error(ret.stdout)
        raise subprocess.CalledProcessError(ret.returncode, command)


def erase_board(dev_id: str | None):
    command = [
        'nrfutil', 'device', 'erase'
    ]
    if dev_id:
        command.extend(['--serial-number', dev_id])
    run_command(command)


def flash_board(build_dir: Path | str, dev_id: str | None, erase: bool = False):
    logger.info("Flash the board.")
    command = [
        'west', 'flash', '--skip-rebuild',
        '-d', str(build_dir)
    ]
    if dev_id:
        command.extend(['--dev-id', dev_id])
    if erase:
        command.extend(['--erase'])
    run_command(command)


def provision_keys_for_kmu(dev_id: str | None, key1: str | Path,
                           key2: str | Path | None = None,
                           key3: str | Path | None = None):
    logger.info("Provision keys using west command. Erase the board first")
    erase_board(dev_id)
    command = [
        'west', 'ncs-provision', 'upload',
        '--soc', 'nrf54l15',
        '--key', str(key1)
    ]
    if key2:
        command.extend(['--key', str(key2)])
    if key3:
        command.extend(['--key', str(key3)])
    if dev_id:
        command.extend(['--dev-id', dev_id])
    run_command(command)
    logger.info("Keys provisioned successfully")
