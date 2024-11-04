# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
from __future__ import annotations

import logging

from pathlib import Path
from twister_harness import DeviceAdapter
from twister_harness.helpers.utils import match_lines, find_in_config
from common import (
    provision_keys_for_kmu,
    flash_board,
    APP_KEYS_FOR_KMU
)

logger = logging.getLogger(__name__)


def test_kmu_correct_keys_uploaded(dut: DeviceAdapter):
    """
    Upload valid keys to DUT using west ncs-provission command
    and verify it in application.
    """
    zephyr_base = find_in_config(dut.device_config.build_dir / 'CMakeCache.txt', 'ZEPHYR_BASE:PATH')
    default_key = Path(zephyr_base).parent / 'bootloader' / 'mcuboot' / 'root-ed25519.pem'
    provision_keys_for_kmu(dut.device_config.id,
                           key1=default_key,
                           key2=APP_KEYS_FOR_KMU / 'root-ed25519-1.pem',
                           key3=APP_KEYS_FOR_KMU / 'root-ed25519-2.pem')

    logger.info("Flash the board once again and check if keys are verified")
    dut.clear_buffer()
    flash_board(dut.device_config.build_dir, dut.device_config.id)

    lines = dut.readlines_until(
        regex='Key 2 failed|Key 2 verified|PSA crypto init failed',
        print_output=True, timeout=20)

    match_lines(lines, [
        'Default key verified',
        'Key 1 verified',
        'Key 2 verified'
    ])


def test_kmu_wrong_keys_uploaded(dut: DeviceAdapter):
    """
    Upload two wrong keys to DUT using west ncs-provission command
    and verify it in application.
    """
    provision_keys_for_kmu(dut.device_config.id,
                           key1=APP_KEYS_FOR_KMU / 'root-ed25519-w.pem',
                           key2=APP_KEYS_FOR_KMU / 'root-ed25519-1.pem',
                           key3=APP_KEYS_FOR_KMU / 'root-ed25519-w.pem')

    logger.info("Flash the board once again and check if keys are verified")
    dut.clear_buffer()
    flash_board(dut.device_config.build_dir, dut.device_config.id)

    lines = dut.readlines_until(
        regex='Key 2 failed|Key 2 verified|PSA crypto init failed',
        print_output=True, timeout=20)

    match_lines(lines, [
        'Default key failed',
        'Key 1 verified',
        'Key 2 failed'
    ])
