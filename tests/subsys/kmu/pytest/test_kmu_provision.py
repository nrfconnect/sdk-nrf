# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
from __future__ import annotations

import logging

from pathlib import Path

import pytest
from twister_harness import DeviceAdapter
from twister_harness.helpers.utils import match_lines, find_in_config
from common import (
    get_keyname_for_mcuboot,
    provision_keys_for_kmu,
    reset_board,
    APP_KEYS_FOR_KMU
)

logger = logging.getLogger(__name__)


@pytest.mark.usefixtures("no_reset")
@pytest.mark.parametrize(
    'test_option', ['one_key', 'three_keys_second_used', 'three_keys_last_used']
)
def test_kmu_use_key_from_config(dut: DeviceAdapter, test_option):
    """
    Upload proper key using west ncs-provision command,
    verify that the application boots successfully.
    """
    logger.info("Provision same key that was used during building")
    sysbuild_config = Path(dut.device_config.build_dir) / 'zephyr' / '.config'
    key_file = find_in_config(sysbuild_config, 'SB_CONFIG_BOOT_SIGNATURE_KEY_FILE')

    if test_option == 'three_keys_second_used':
        keys = [
            APP_KEYS_FOR_KMU / 'root-ed25519-1.pem',
            key_file,
            APP_KEYS_FOR_KMU / 'root-ed25519-2.pem'
        ]
    elif test_option == 'three_keys_last_used':
        keys = [
            APP_KEYS_FOR_KMU / 'root-ed25519-1.pem',
            APP_KEYS_FOR_KMU / 'root-ed25519-2.pem',
            key_file
        ]
    else:
        keys = [key_file]

    provision_keys_for_kmu(
        keys=keys,
        keyname=get_keyname_for_mcuboot(sysbuild_config),
        dev_id=dut.device_config.id
    )
    dut.clear_buffer()
    reset_board(dut.device_config.id)

    lines = dut.readlines_until(
        regex='Unable to find bootable image|Jumping to the first image slot',
        print_output=True, timeout=20)

    match_lines(lines, ['Jumping to the first image slot'])
    logger.info("Passed: Booted successfully after provisioning the same key that was used during building")


@pytest.mark.usefixtures("no_reset")
def test_kmu_use_wrong_key(dut: DeviceAdapter):
    """
    Upload keys using west ncs-provision command,
    verify that the application does not boot if the keys are incorrect.
    """
    logger.info("Provision wrong keys")
    sysbuild_config = Path(dut.device_config.build_dir) / 'zephyr' / '.config'
    provision_keys_for_kmu(
        keys=[
            APP_KEYS_FOR_KMU / 'root-ed25519-1.pem',
            APP_KEYS_FOR_KMU / 'root-ed25519-2.pem',
            APP_KEYS_FOR_KMU / 'root-ed25519-w.pem'
        ],
        keyname=get_keyname_for_mcuboot(sysbuild_config),
        dev_id=dut.device_config.id
    )

    dut.clear_buffer()
    reset_board(dut.device_config.id)

    lines = dut.readlines_until(
        regex='Unable to find bootable image|Jumping to the first image slot',
        print_output=True, timeout=20)

    match_lines(lines, [
        'ED25519 signature verification failed',
        'Image in the primary slot is not valid',
        'Unable to find bootable image'
    ])
    logger.info("Passed: Not booted when used wrong keys")
