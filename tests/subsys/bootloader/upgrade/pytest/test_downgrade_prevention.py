# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Test for hardware downgrade prevention mechanism in MCUboot."""

from __future__ import annotations

import logging
from pathlib import Path

import pytest
from helpers import reset_board
from twister_harness import DeviceAdapter, MCUmgr, Shell
from twister_harness.helpers.utils import find_in_config
from upgrade_test_manager import UpgradeTestWithMCUmgr

logger = logging.getLogger(__name__)


def check_sw_downgrade_prevention_is_not_configured(dut: DeviceAdapter) -> None:
    """Check that software downgrade prevention is not configured."""
    # Ensure proper configuration
    mcuboot_config: Path = dut.device_config.build_dir / "mcuboot" / "zephyr" / ".config"
    assert not find_in_config(mcuboot_config, "CONFIG_MCUBOOT_DOWNGRADE_PREVENTION")


def test_hw_downgrade_prevention_with_sign_version(
    dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr
):
    """Verify that the application is not downgraded.

    APP based on smp_svr, MCUboot is the primary bootloader.
    Hardware downgrade prevention mechanism is enabled.
    Upload the application image signed with a lower version.
    Test or confirm image. Reset DUT.
    Verify that the application is not downgraded.
    """
    check_sw_downgrade_prevention_is_not_configured(dut)
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.decrease_version()
    updated_app = tm.generate_image()
    tm.run_upgrade(updated_app)

    tm.verify_after_reset(
        lines=[
            "Swap type: test",
            "Image in the secondary slot is not valid",
            tm.get_version_string_to_verify_in_bootlog(tm.origin_mcuboot_version),
        ]
    )
    tm.check_with_shell_command(tm.origin_mcuboot_version)
    logger.info("PASSED: Application is not downgraded when signed with a lower version.")


def get_hw_counter_values(dut: DeviceAdapter) -> tuple[int, int]:
    """Get the current hardware counter value and the number of slots."""
    config_path = dut.device_config.build_dir / "zephyr" / ".config"
    counter_slots = int(
        find_in_config(config_path, "SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_SLOTS")
    )
    current_counter_value = int(
        find_in_config(config_path, "SB_CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION_COUNTER_VALUE")
    )
    return counter_slots, current_counter_value


def test_hw_downgrade_prevention_with_monotonic_counters(
    dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr
):
    """Verify that APP is not downgraded when monotonic counter is lower than the current one."""
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)

    _, current_counter_value = get_hw_counter_values(dut)

    if current_counter_value == 1:
        # If the current counter value is 1, we cannot decrease, do one upgrade
        # and then run downgrade prevention test.
        logger.info("Current counter value is 1, performing an upgrade to increase it.")
        current_counter_value += 1
        tm.set_hw_counter_value(current_counter_value)
        updated_app = tm.generate_image()
        tm.run_upgrade(updated_app, confirm=True)
        tm.verify_swap_confirmed_after_reset()

    tm.increase_version()
    current_counter_value -= 1
    tm.set_hw_counter_value(current_counter_value)
    updated_app = tm.generate_image()
    tm.run_upgrade(updated_app, confirm=True)

    tm.verify_after_reset(
        lines=[
            "Swap type: perm",
            "Image in the secondary slot is not valid",
            tm.get_version_string_to_verify_in_bootlog(tm.origin_mcuboot_version),
        ]
    )
    tm.check_with_shell_command(tm.origin_mcuboot_version)
    logger.info(
        "PASSED: Application is not downgraded when monotonic counter is lower than the current "
        "one."
    )


@pytest.mark.nightly
def test_monotonic_counter_prevents_upgrade(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that APP is not upgraded when monotonic counter value is greater than number of
    slots."""
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)

    counter_slots, current_counter_value = get_hw_counter_values(dut)
    current_sign_version = tm.get_current_sign_version()

    for current_slot in range(1, counter_slots + 1):
        logger.info(f"Performing upgrade for slot {current_slot}")
        tm.increase_version()
        current_counter_value += 1
        tm.set_hw_counter_value(current_counter_value)
        updated_app = tm.generate_image()
        tm.run_upgrade(updated_app, confirm=True)
        if current_slot == counter_slots:
            break
        tm.verify_swap_confirmed_after_reset()
        current_sign_version = tm.get_current_sign_version()

    # monotonic counter value is now greater than number of slots, app should not be upgraded
    tm.verify_after_reset(
        lines=[
            "Swap type: perm",
            "E: Security counter update",
            "E: Image in the secondary slot is not valid",
            tm.get_version_string_to_verify_in_bootlog(current_sign_version),
        ]
    )
    logger.info("Reset DUT to verify that the app is alive and not upgraded.")
    reset_board(dut.device_config.id)
    tm.verify_after_reset(lines=[tm.get_version_string_to_verify_in_bootlog(current_sign_version)])
    logger.info("PASSED: App not upgraded, monotonic counter prevents upgrade.")
