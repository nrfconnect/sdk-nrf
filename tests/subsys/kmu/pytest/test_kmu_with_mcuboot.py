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


def test_kmu_use_key_from_config(dut: DeviceAdapter):
    """
    Upload proper key using west ncs-provision command,
    verify that the application boots successfully.
    """
    logger.info("Provision same key that was used during building")
    signature_key_file = find_in_config(Path(dut.device_config.build_dir) / 'mcuboot' / 'zephyr' / '.config',
                                        'CONFIG_BOOT_SIGNATURE_KEY_FILE')
    provision_keys_for_kmu(dut.device_config.id, key1=signature_key_file)

    dut.clear_buffer()
    flash_board(dut.device_config.build_dir, dut.device_config.id)

    lines = dut.readlines_until(
        regex='Unable to find bootable image|Jumping to the first image slot',
        print_output=True, timeout=20)

    match_lines(lines, ['Jumping to the first image slot'])
    logger.info("Passed: Booted succesvully after provisioning the same key that was used during building")


def test_kmu_use_predefined_keys(dut: DeviceAdapter):
    """
    Upload keys using west ncs-provision command,
    verify that the application boots successfully if the keys are correct,
    and does not boot if the keys are incorrect.
    """
    signature_key_file = find_in_config(Path(dut.device_config.build_dir) / 'mcuboot' / 'zephyr' / '.config',
                                        'CONFIG_BOOT_SIGNATURE_KEY_FILE')
    zephyr_base = find_in_config(dut.device_config.build_dir / 'CMakeCache.txt', 'ZEPHYR_BASE:PATH')
    default_key = Path(zephyr_base).parent / 'bootloader' / 'mcuboot' / 'root-ed25519.pem'
    provision_keys_for_kmu(dut.device_config.id,
                           key1=default_key,
                           key2=APP_KEYS_FOR_KMU / 'root-ed25519-1.pem',
                           key3=APP_KEYS_FOR_KMU / 'root-ed25519-2.pem')

    dut.clear_buffer()
    flash_board(dut.device_config.build_dir, dut.device_config.id)

    lines = dut.readlines_until(
        regex='Unable to find bootable image|Jumping to the first image slot',
        print_output=True, timeout=20)

    if 'root-ed25519-w.pem' in signature_key_file:
        match_lines(lines, [
            'ED25519 signature verification failed',
            'Image in the primary slot is not valid',
            'Unable to find bootable image'
        ])
        logger.info("Passed: Not booted when used wrong keys")
    else:
        match_lines(lines, ['Jumping to the first image slot'])
        logger.info("Passed: Booted with correct keys")
