# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
from __future__ import annotations

import logging

from pathlib import Path

import pytest
from twister_harness import DeviceAdapter
from twister_harness.helpers.utils import match_lines, match_no_lines, find_in_config
from common import (
    provision_keys_for_kmu,
    reset_board,
    APP_KEYS_FOR_KMU
)

logger = logging.getLogger(__name__)


def test_kmu_policy_revokable(dut: DeviceAdapter):
    """
    Upload keys using 'revokable' policy,
    revoke keys and verify that the device does not boot.
    """
    logger.info("Provision keys with 'revokable' policy")
    sysbuild_config = Path(dut.device_config.build_dir) / 'zephyr' / '.config'
    key_file = find_in_config(sysbuild_config, 'SB_CONFIG_BOOT_SIGNATURE_KEY_FILE')
    provision_keys_for_kmu(
        keys=[key_file],
        keyname="UROT_PUBKEY",
        policy='revokable',
        dev_id=dut.device_config.id
    )
    dut.clear_buffer()
    reset_board(dut.device_config.id)

    lines = dut.readlines_until(
        regex='Unable to find bootable image|Destroy ok',
        print_output=True, timeout=20)
    match_lines(lines, ['Destroy ok'])
    match_no_lines(lines, ['Unable to find bootable image'])
    logger.info("Revoked keys, reboot once again")

    dut.clear_buffer()
    reset_board(dut.device_config.id)
    lines = dut.readlines_until(
        regex='Unable to find bootable image|Destroy ok|Destroy failed',
        print_output=True, timeout=20)
    match_lines(lines, ['Unable to find bootable image'])
    logger.info("Passed: not booted with revoked keys")


def test_kmu_policy_lock(dut: DeviceAdapter):
    """
    Upload keys using 'lock' policy,
    try to revoke keys and verify that keys are not revoked
    and the device boots successfully.
    """
    logger.info("Provision keys with 'lock' policy")
    sysbuild_config = Path(dut.device_config.build_dir) / 'zephyr' / '.config'
    key_file = find_in_config(sysbuild_config, 'SB_CONFIG_BOOT_SIGNATURE_KEY_FILE')
    provision_keys_for_kmu(
        keys=[key_file],
        keyname="UROT_PUBKEY",
        policy='lock',
        dev_id=dut.device_config.id
    )
    dut.clear_buffer()
    reset_board(dut.device_config.id)

    lines = dut.readlines_until(
        regex='Unable to find bootable image|Destroy ok|Destroy failed',
        print_output=True, timeout=20)
    match_lines(lines, ['Destroy failed'])
    match_no_lines(lines, ['Unable to find bootable image'])
    logger.info("Keys not destroyed, reboot once again")

    dut.clear_buffer()
    reset_board(dut.device_config.id)
    lines = dut.readlines_until(
        regex='Unable to find bootable image|Destroy ok|Destroy failed',
        print_output=True, timeout=20)
    match_no_lines(lines, ['Unable to find bootable image'])
    logger.info("Passed: locked keys not destroyed, booted successfully")


@pytest.mark.parametrize(
    'test_option', ['use_last_key', 'use_revoked_key']
)
def test_kmu_policy_lock_last(dut: DeviceAdapter, test_option):
    """
    Upload keys using 'lock-last' policy,
    try to revoke keys and verify that last keys are not revoked,
    and the device boots successfully if last keys is used
    and not booted if revoked key is used.
    """
    logger.info("Provision keys with revokable policy")
    sysbuild_config = Path(dut.device_config.build_dir) / 'zephyr' / '.config'
    key_file = find_in_config(sysbuild_config, 'SB_CONFIG_BOOT_SIGNATURE_KEY_FILE')

    if test_option == 'use_last_key':
        keys = [
            APP_KEYS_FOR_KMU / 'root-ed25519-1.pem',
            APP_KEYS_FOR_KMU / 'root-ed25519-2.pem',
            key_file
        ]
    else:
        keys = [
            key_file,
            APP_KEYS_FOR_KMU / 'root-ed25519-1.pem',
            APP_KEYS_FOR_KMU / 'root-ed25519-2.pem'
        ]

    provision_keys_for_kmu(
        keys=keys,
        keyname="UROT_PUBKEY",
        policy='lock-last',
        dev_id=dut.device_config.id
    )
    dut.clear_buffer()
    reset_board(dut.device_config.id)

    lines = dut.readlines_until(
        regex='Unable to find bootable image|Destroy failed',
        print_output=True, timeout=20)
    match_lines(lines, ['Destroy ok', 'Destroy failed'])
    match_no_lines(lines, ['Unable to find bootable image'])
    logger.info("Revoked keys but not all, reboot once again")

    dut.clear_buffer()
    reset_board(dut.device_config.id)

    lines = dut.readlines_until(
        regex='Unable to find bootable image|Destroy ok|Destroy failed',
        print_output=True, timeout=20)

    if test_option == 'use_last_key':
        match_no_lines(lines, ['Unable to find bootable image'])
        logger.info("Passed: last key not destroyed, booted successfully")
    else:
        match_lines(lines, ['Unable to find bootable image'])
        logger.info("Passed: not booted with revoked key")
