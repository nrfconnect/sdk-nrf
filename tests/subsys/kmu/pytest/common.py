# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
from __future__ import annotations

import logging
import os
import shlex
import subprocess

from pathlib import Path

logger = logging.getLogger(__name__)

APP_KEYS_FOR_KMU = Path(__file__).resolve().parent.parent / 'keys'


def normalize_path(path: str | None) -> str:
    if path is not None:
        path = os.path.expanduser(os.path.expandvars(path))
        path = os.path.normpath(os.path.abspath(path))
    return path


def run_command(command: list[str], timeout: int = 30):
    logger.info(f"CMD: {shlex.join(command)}")
    ret: subprocess.CompletedProcess = subprocess.run(
        command, text=True, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, timeout=timeout)
    if ret.returncode:
        logger.error(f"Failed command: {shlex.join(command)}")
        logger.info(ret.stdout)
        raise subprocess.CalledProcessError(ret.returncode, command)


def reset_board(dev_id: str | None = None):
    command = [
        'nrfutil', 'device', 'reset'
    ]
    if dev_id:
        command.extend(['--serial-number', dev_id])
    run_command(command)


def erase_board(dev_id: str | None):
    command = [
        'nrfutil', 'device', 'erase'
    ]
    if dev_id:
        command.extend(['--serial-number', dev_id])
    run_command(command)


def flash_with_nrfutil(firmware: Path | str, dev_id: str):
    command = [
        'nrfutil', 'device', 'program',
        '--firmware', str(firmware)
    ]
    if dev_id:
        command.extend(['--serial-number', dev_id])
    run_command(command)


def flash_board(build_dir: Path | str, dev_id: str | None, erase: bool = False):
    command = [
        'west', 'flash', '--skip-rebuild',
        '-d', str(build_dir)
    ]
    if dev_id:
        command.extend(['--dev-id', dev_id])
    if erase:
        command.extend(['--erase'])
    run_command(command)


def provision_keys_for_kmu(
        keys: list[str] | str,
        keyname: str = "UROT_PUBKEY",  # UROT_PUBKEY, BL_PUBKEY, APP_PUBKEY
        soc: str = "nrf54l15",  # nrf54l15, nrf54l10, nrf54l05
        policy: str | None = None,  # revokable, lock, lock-last (default)
        dev_id: str | None = None
):
    logger.info("Provision keys using west command.")
    command = [
        'west', 'ncs-provision', 'upload',
        '--keyname', keyname,
        '--soc', soc
    ]
    if policy:
        command += ['--policy', policy]
    if dev_id:
        command += ['--dev-id', dev_id]
    if not isinstance(keys, list):
        keys = [keys]
    for key in keys:
        command += ['--key', normalize_path(str(key))]

    run_command(command)
    logger.info("Keys provisioned successfully")
