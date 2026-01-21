# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
from __future__ import annotations

import logging
import os
import shlex
import subprocess
from pathlib import Path

from twister_harness.helpers.utils import find_in_config

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


def get_keyname_for_mcuboot(sysbuild_config: Path) -> str:
    keyname = "BL_PUBKEY"
    if (find_in_config(sysbuild_config, "SB_CONFIG_SECURE_BOOT_APPCORE")
            or find_in_config(sysbuild_config, "SB_CONFIG_MCUBOOT_SIGNATURE_KMU_UROT_MAPPING")):
        keyname = "UROT_PUBKEY"
    return keyname


def provision_keys_for_kmu(
        keys: list[str] | str,
        keyname: str = "UROT_PUBKEY",  # UROT_PUBKEY, BL_PUBKEY, APP_PUBKEY
        policy: str | None = None,  # revokable, lock, lock-last (default)
        dev_id: str | None = None
):
    logger.info("Provision keys using west command.")
    command = [
        'west', 'ncs-provision', 'upload',
        '--keyname', keyname
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
