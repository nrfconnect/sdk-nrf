# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Tests for MCUboot application upgrade and downgrade scenarios."""

from __future__ import annotations

import logging

import pytest
from twister_harness import DeviceAdapter, MCUmgr, Shell
from upgrade_test_manager import UpgradeTestWithMCUmgr

logger = logging.getLogger(__name__)


def test_upgrade_with_confirm(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that the application can be updated.

    APP based on smp_svr, MCUboot is the primary bootloader.
    Upload the application image signed with a higher version.
        Test image. Reset DUT.
    Verify that the updated application is booted.
        Confirm image. Reset DUT.
    Verify that the new application is still booted.
    """
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.increase_version()
    updated_app = tm.generate_image()
    tm.run_upgrade(updated_app)
    tm.verify_swap_test_after_reset()
    assert shell.wait_for_prompt(timeout=10.0), "No shell prompt after upgrade"

    tm.image_confirm()
    tm.reset_device()
    tm.verify_no_swap_after_reset()


@pytest.mark.usefixtures("kmu_provision")
def test_upgrade_with_revert(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that MCUboot will roll back an image that is not confirmed.

    APP based on smp_svr, MCUboot is the primary bootloader.
    Upload the application image signed with a higher version.
        Test image. Reset DUT.
    Verify that the updated application is booted.
        Reset DUT without confirming the image.
    Verify that MCUboot reverts update.
    """
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.increase_version()
    updated_app = tm.generate_image()
    tm.run_upgrade(updated_app)
    tm.verify_swap_test_after_reset()
    assert shell.wait_for_prompt(timeout=10.0), "No shell prompt after upgrade"

    logger.info("Revert images")
    tm.reset_device()

    logger.info("Verify APP is reverted")
    tm.verify_swap_in_boot_log(swap_type="revert", version=tm.origin_mcuboot_version)


def test_sw_downgrade_prevention(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that the application is not downgraded.

    APP based on smp_svr, MCUboot is the primary bootloader.
        Software downgrade prevention mechanism is enabled.
    Upload the application image signed with a lower version.
        Test or confirm image. Reset DUT.
    Verify that downgrade prevention mechanism blocked the image swap.
    """
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.decrease_version()
    updated_app = tm.generate_image()
    tm.run_upgrade(updated_app)
    tm.verify_after_reset(
        lines=[
            "Swap type: test",
            "erased due to downgrade prevention",
            tm.get_version_string_to_verify_in_bootlog(tm.origin_mcuboot_version),
        ]
    )


@pytest.mark.nightly
def test_upgrade_multiple_times(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that the application can be updated multiple times.

    APP based on smp_svr, MCUboot is the primary bootloader.
    Upload the application image signed with a higher version.
        Confirm image. Reset DUT.
        Verify that the updated application is booted.
    Repeat the process.
    """
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    for _ in range(2):
        tm.increase_version()
        updated_app = tm.generate_image()
        tm.run_upgrade(updated_app, confirm=True)
        tm.verify_swap_confirmed_after_reset()
        assert shell.wait_for_prompt(timeout=10.0), "No shell prompt after upgrade"

    tm.reset_device_from_shell()
    tm.verify_no_swap_after_reset()
