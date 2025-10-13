# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Test for hardware downgrade prevention mechanism in NSIB/B0."""

from __future__ import annotations

import logging

import pytest
from required_build import get_required_images_to_update
from twister_harness import DeviceAdapter, MCUmgr, Shell
from twister_harness.helpers.utils import find_in_config
from upgrade_test_manager import UpgradeTestWithMCUmgr

logger = logging.getLogger(__name__)


def test_b0_firmware_version_blocks_downgrade(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that APP is not downgraded when firmware version is lower than the current one."""
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)

    current_firmware_version = int(
        find_in_config(
            dut.device_config.build_dir / "mcuboot" / "zephyr" / ".config", "CONFIG_FW_INFO_FIRMWARE_VERSION"
        )
    )

    if current_firmware_version == 1:
        pytest.fail(
            "Current firmware version value is 1, which is not allowed for this test. "
            "Please set a higher initial value in the configuration."
        )

    downgraded_firmware_version = current_firmware_version - 1
    _, _, s1_image = get_required_images_to_update(
        dut, sign_version=tm.get_current_sign_version(), firmware_version=downgraded_firmware_version
    )

    tm.run_upgrade(s1_image, confirm=True)
    tm.verify_after_reset(
        lines=["Attempting to boot slot 0"],
        no_lines=["Attempting to boot slot 1", f"Firmware version {downgraded_firmware_version}"],
    )
    assert shell.wait_for_prompt(timeout=3.0), "No shell prompt after downgrade"
    tm.reset_device_from_shell()

    tm.verify_after_reset(
        lines=["Attempting to boot slot 0", f"Firmware version {current_firmware_version}"],
        no_lines=["Attempting to boot slot 1", f"Firmware version {downgraded_firmware_version}"],
    )
    logger.info("PASSED: Application is not downgraded when firmware version is lower than the current one.")


@pytest.mark.nightly
@pytest.mark.xfail(reason="Quarantine: NCSDK-31918 Monotonic counter update protection does not work")
def test_b0_monotonic_counters_limit_number_of_upgrades(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that number of upgrades is limited by monotonic counters."""
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)

    current_firmware_version = int(
        find_in_config(
            dut.device_config.build_dir / "mcuboot" / "zephyr" / ".config", "CONFIG_FW_INFO_FIRMWARE_VERSION"
        )
    )
    current_firmware_version += 1
    _, _, s1_image = get_required_images_to_update(
        dut, sign_version=tm.get_current_sign_version(), firmware_version=current_firmware_version
    )

    logger.info(f"Upgrade from S1 slot, firmware version: {current_firmware_version}")
    tm.run_upgrade(s1_image, confirm=True)
    tm.verify_after_reset(
        lines=[
            "Attempting to boot slot 1",
            f"Firmware version {current_firmware_version}",
            "Setting monotonic counter",
        ],
        no_lines=["Failed during setting the monotonic counter"],
    )
    assert shell.wait_for_prompt(timeout=3.0), "No shell prompt after upgrade"

    current_firmware_version += 1
    _, _, s1_image = get_required_images_to_update(
        dut, sign_version=tm.get_current_sign_version(), firmware_version=current_firmware_version
    )
    s0_image = s1_image.parent / "signed_by_mcuboot_and_b0_mcuboot.bin"

    logger.info(f"Upgrade from S0 slot, firmware version: {current_firmware_version}")
    tm.run_upgrade(s0_image, confirm=True)
    tm.verify_after_reset(
        lines=["Failed during setting the monotonic counter"],
        # no_lines=[f"Firmware version {current_firmware_version}"]
    )
    # This test can be finished here, but we commented the line to check Firmware version
    # to ensure, that the error message is printed and to check what happens after reset.
    assert shell.wait_for_prompt(timeout=3.0), "No shell prompt"
    tm.reset_device_from_shell()

    tm.verify_after_reset(
        lines=["Failed during setting the monotonic counter"], no_lines=[f"Firmware version {current_firmware_version}"]
    )
    logger.info("PASSED: Number of upgrades is limited by monotonic counters.")
