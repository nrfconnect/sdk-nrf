# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Tests for MCUboot KMU key provisioning and key revocation scenarios."""

from __future__ import annotations

import logging
from collections.abc import Generator
from pathlib import Path

import pytest
from helpers import reset_board
from key_provisioning import APP_KEYS_FOR_KMU, get_keyname_for_mcuboot, provision_keys_for_kmu
from twister_harness import DeviceAdapter, MCUmgr, Shell
from twister_harness.fixtures import determine_scope
from twister_harness.helpers.utils import find_in_config, match_lines, match_no_lines
from upgrade_test_manager import UpgradeTestWithMCUmgr

logger = logging.getLogger(__name__)


@pytest.fixture(scope=determine_scope)
def kmu_provision_mcuboot(dut: DeviceAdapter) -> Generator[DeviceAdapter, None, None]:  # type: ignore
    """Provision MCUboot keys using west ncs-provision upload command."""
    sysbuild_config = Path(dut.device_config.build_dir) / "zephyr" / ".config"
    assert find_in_config(sysbuild_config, "SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU"), (
        "KMU keys not configured"
    )

    lines = dut.readlines_until(
        regex="Unable to find bootable image|Jumping to the first image slot",
        print_output=True,
        timeout=20,
    )
    assert not any("Jumping to the first image slot" in line for line in lines), (
        "MCUboot: already provisioned"
    )

    logger.info(
        "Provision KMU keys for MCUboot, Second key is a current key.First key should be revoked."
    )
    key_file = find_in_config(sysbuild_config, "SB_CONFIG_BOOT_SIGNATURE_KEY_FILE")
    keys = [
        APP_KEYS_FOR_KMU / "root-ed25519-1.pem",
        key_file,
        APP_KEYS_FOR_KMU / "root-ed25519-2.pem",
    ]

    keyname = get_keyname_for_mcuboot(sysbuild_config)
    provision_keys_for_kmu(keys=keys, keyname=keyname, dev_id=dut.device_config.id)
    dut.clear_buffer()
    reset_board(dut.device_config.id)


@pytest.mark.usefixtures("kmu_provision_mcuboot")
def test_kmu_revoke_old_keys(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that MCUboot disables old keys when updates comes with new valid key.

    Upgrading with revoked keys should be rejected.
    """
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.increase_version()
    logger.info("Sign image with first provisioned key, that should be revoked")
    tm.build_params.imgtool_params.key_file = APP_KEYS_FOR_KMU / "root-ed25519-1.pem"
    updated_app = tm.generate_image()
    tm.run_upgrade(updated_app)

    lines = dut.readlines_until(
        regex="Unable to find bootable image|Jumping to the first image slot",
        print_output=True,
        timeout=20,
    )

    match_lines(
        lines,
        ["Image in the secondary slot is not valid", "Jumping to the first image slot"],
    )
    tm.check_with_shell_command(tm.origin_mcuboot_version)
    logger.info("Passed: MCUboot rejected image signed with revoked key")


@pytest.mark.nightly
@pytest.mark.usefixtures("kmu_provision_mcuboot")
def test_kmu_upgrade_with_new_key_then_with_old(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that MCUboot accepts image signed with new key, that was previously provisioned.

    Then update with revoked key and verify that it is rejected.
    """
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)

    sysbuild_config = Path(dut.device_config.build_dir) / "zephyr" / ".config"
    default_key_file = find_in_config(sysbuild_config, "SB_CONFIG_BOOT_SIGNATURE_KEY_FILE")

    tm.increase_version()
    logger.info("Sign image with third key")
    tm.build_params.imgtool_params.key_file = APP_KEYS_FOR_KMU / "root-ed25519-2.pem"
    updated_app = tm.generate_image()
    tm.run_upgrade(updated_app, confirm=True)

    lines = dut.readlines_until(
        regex="Unable to find bootable image|Jumping to the first image slot",
        print_output=True,
        timeout=20,
    )
    match_no_lines(lines, ["Unable to find bootable image"])

    tm.check_with_shell_command()
    logger.info("Passed step 1: MCUboot accepted image signed with not revoked key")

    logger.info("Reset DUT to revoke old keys (swap type: none must be applied)")
    tm.reset_device_from_shell()
    lines = dut.readlines_until(
        regex="Jumping to the first image slot", print_output=True, timeout=20
    )
    match_lines(lines, ["Swap type: none"])

    logger.info("Sign image with second provisioned key, that should be revoked")
    mcuboot_version = tm.get_current_sign_version()
    tm.increase_version()
    tm.build_params.imgtool_params.key_file = default_key_file  # type: ignore
    updated_app = tm.generate_image()
    tm.run_upgrade(updated_app)

    lines = dut.readlines_until(
        regex="Unable to find bootable image|Jumping to the first image slot",
        print_output=True,
        timeout=20,
    )

    match_lines(
        lines,
        ["Image in the secondary slot is not valid", "Jumping to the first image slot"],
    )
    tm.check_with_shell_command(mcuboot_version)
    logger.info("Passed: MCUboot rejected image signed with revoked key")
