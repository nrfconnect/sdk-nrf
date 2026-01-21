# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Tests for MCUboot upgrade and downgrade prevention with NSIB as primary bootloader."""

from __future__ import annotations

import logging

import pytest
from required_build import get_required_images_to_update
from twister_harness import DeviceAdapter, MCUmgr, Shell
from twister_harness.helpers.utils import find_in_config
from upgrade_test_manager import UpgradeTestWithMCUmgr

logger = logging.getLogger(__name__)


class UpgradeMCUBootUseMCUmgr(UpgradeTestWithMCUmgr):
    """Upgrade test manager for MCUboot with NSIB as primary bootloader."""

    def __init__(self, dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
        """Initialize UpgradeMCUBootUseMCUmgr with DUT, shell, and MCUmgr."""
        super().__init__(dut, shell, mcumgr)
        self.image_index = self._get_image_index()

    def _get_image_index(self) -> int:
        """Get image index from build parameters or config."""
        if self.build_params.sysbuild:
            image_index = find_in_config(
                self.build_params.build_dir / "mcuboot" / "zephyr" / ".config",
                "CONFIG_MCUBOOT_MCUBOOT_IMAGE_NUMBER",
            )
            if image_index and int(image_index) > 0:
                return int(image_index)
        return 0


@pytest.mark.usefixtures("kmu_provision")
def test_upgrade_of_mcuboot_with_nsib(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that the MCUboot firmware is updated.

    APP based on smp_svr, NSIB is used as a primary bootloader,
    MCUboot is a second stage bootloader.
    Upload MCUboot firmware with a higher version.
    Confirm image. Reset DUT.
    Verify that the MCUboot is updated.
    """
    tm = UpgradeMCUBootUseMCUmgr(dut, shell, mcumgr)
    tm.increase_version()

    _, _, s1_image = get_required_images_to_update(
        dut,
        sign_version=tm.get_current_sign_version(),
        firmware_version=3,
        netcore_name=tm.build_params.net_core_name,
    )

    tm.run_upgrade(s1_image, confirm=True)

    tm.verify_after_reset(
        lines=[
            f"Image index: {tm.image_index}, Swap type: perm",
        ],
        no_lines=["insufficient version in secondary slot"],
    )
    assert shell.wait_for_prompt(timeout=10.0), "No shell prompt after upgrade"
    logger.info(
        "Firmware was successfully updated. Reset once again to check current firmware version"
    )
    tm.reset_device()
    tm.verify_after_reset(lines=["Firmware version 3"])
    logger.info("Firmware version is correct")


@pytest.mark.usefixtures("kmu_provision")
def test_sw_downgrade_prevention_of_mcuboot_with_nsib(
    dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr
):
    """Verify that the MCUboot firmware is not downgraded.

    APP based on smp_svr, NSIB is used as a primary bootloader,
    MCUboot is a second stage bootloader.
    Upload MCUboot firmware with a lower version.
    Confirm images. Reset DUT.
    Verify that the MCUboot is not downgraded.
    """
    tm = UpgradeMCUBootUseMCUmgr(dut, shell, mcumgr)
    tm.decrease_version()

    _, _, s1_image = get_required_images_to_update(
        dut,
        sign_version=tm.get_current_sign_version(),
        firmware_version=1,
        netcore_name=tm.build_params.net_core_name,
    )

    tm.run_upgrade(s1_image, confirm=True)

    # to be checked, for sysbuild, image is copied to secondary slot
    # but not used after reset .. without any log!
    # the only difference is log after successful upgrade:
    #    'Setting monotonic counter (version: 3, slot: 1)'
    tm.verify_after_reset(
        lines=["Firmware version 2", f"Image index: {tm.image_index}, Swap type: perm"]
    )
    assert shell.wait_for_prompt(timeout=10.0), "No shell prompt after upgrade"
    logger.info("Firmware was not downgraded. Reset once again to check current firmware version")

    tm.reset_device()
    tm.verify_after_reset(lines=["Firmware version 2"])
    logger.info("Firmware version is correct")
