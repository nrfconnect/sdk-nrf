# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Test MCUboot software downgrade prevention with build number comparison (smp_svr_mini_boot)."""

from __future__ import annotations

import logging
from pathlib import Path

import pytest
from twister_harness import DeviceAdapter, MCUmgr, Shell
from twister_harness.helpers.mcumgr import MCUmgrImage
from twister_harness.helpers.utils import find_in_config
from upgrade_test_manager import UpgradeTestWithMCUmgr, use_serial

logger = logging.getLogger(__name__)

BASE_VERSION = "1.2.3+10"
LOWER_BUILD_VERSION = "1.2.3+9"
HIGHER_BUILD_VERSION = "1.2.3+11"


@pytest.fixture(autouse=True)
def use_required_application_build_dir(dut: DeviceAdapter) -> None:
    """Point build_dir to the smp_svr_mini_boot build (test scenario is not built)."""
    if dut.device_config.current_build_dir:
        dut.device_config.build_dir = dut.device_config.current_build_dir


class MiniBootUpgradeTest(UpgradeTestWithMCUmgr):
    """Upgrade test manager for smp_svr_mini_boot."""

    # MCUboot is built without logs, so detect boot by the application banner
    welcome_str = "smp_sample: build time"

    @use_serial
    def get_image_list(self) -> list[MCUmgrImage]:
        """Get image list using MCUmgr."""
        return self.mcumgr.get_image_list()

    def sign(self, version: str) -> Path:
        """Sign the application with the given version, keeping other parameters unchanged."""
        self.build_params.imgtool_params.version = version
        assert self.build_params.imgtool_params.security_counter is None
        return self.generate_image()

    def reset_and_wait_for_app(self) -> None:
        """Reset DUT and wait until the application is booted."""
        self.clear_buffer()
        self.reset_device()
        self.verify_after_reset()


def to_mcumgr_version(version: str) -> str:
    """Convert imgtool version string to the format reported by MCUmgr."""
    return version.replace("+", ".")


def check_build_number_downgrade_prevention_config(dut: DeviceAdapter) -> None:
    """Check that SW downgrade prevention compares image versions including build number."""
    mcuboot_config = dut.device_config.build_dir / "mcuboot" / "zephyr" / ".config"
    assert find_in_config(mcuboot_config, "CONFIG_MCUBOOT_DOWNGRADE_PREVENTION") == "y"
    assert find_in_config(mcuboot_config, "CONFIG_BOOT_VERSION_CMP_USE_BUILD_NUMBER") == "y"
    assert not find_in_config(
        mcuboot_config, "CONFIG_MCUBOOT_DOWNGRADE_PREVENTION_SECURITY_COUNTER"
    )
    assert not find_in_config(mcuboot_config, "CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION")


def start_from_base_version(tm: MiniBootUpgradeTest) -> None:
    """Install and confirm the base version, so each test starts from the same state."""
    logger.info(f"Install base version {BASE_VERSION}")
    tm.clear_buffer()
    tm.run_upgrade(tm.sign(BASE_VERSION), confirm=True)
    tm.verify_after_reset()
    tm.shell.wait_for_prompt(timeout=3)
    tm.check_with_mcumgr_command(BASE_VERSION)


def upload_and_test(tm: MiniBootUpgradeTest, version: str) -> None:
    """Upload image with the given version, mark it for test swap and reset DUT."""
    tm.image_upload(tm.sign(version))
    tm.image_test()
    secondary = [image for image in tm.get_image_list() if image.slot == 1]
    assert len(secondary) == 1, f"Uploaded image not found in secondary slot: {secondary}"
    assert secondary[0].version == to_mcumgr_version(version)
    assert "pending" in secondary[0].flags
    tm.reset_and_wait_for_app()


def test_downgrade_prevention_rejects_lower_build_number(
    dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr
):
    """Verify that an image with a lower build number is rejected.

    MCUboot uses SW downgrade prevention based on image version,
    including the build number.
    Install and confirm version 1.2.3+10.
    Upload version 1.2.3+9, test it and reset DUT.
    Verify that 1.2.3+10 is still running and 1.2.3+9 is removed from the secondary slot.
    """
    check_build_number_downgrade_prevention_config(dut)
    tm = MiniBootUpgradeTest(dut, shell, mcumgr)
    start_from_base_version(tm)

    upload_and_test(tm, LOWER_BUILD_VERSION)

    shell.wait_for_prompt(timeout=3)
    tm.check_with_mcumgr_command(BASE_VERSION)
    assert not [image for image in tm.get_image_list() if image.slot == 1], (
        "Image with lower build number was not erased from the secondary slot"
    )
    logger.info("PASSED: Image with lower build number rejected by downgrade prevention.")


def test_downgrade_prevention_accepts_higher_build_number(
    dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr
):
    """Verify that an image with a higher build number is accepted.

    MCUboot uses SW downgrade prevention based on image version,
    including the build number.
    Install and confirm version 1.2.3+10.
    Upload version 1.2.3+11, test it and reset DUT.
    Verify that 1.2.3+11 is running.
    """
    check_build_number_downgrade_prevention_config(dut)
    tm = MiniBootUpgradeTest(dut, shell, mcumgr)
    start_from_base_version(tm)

    upload_and_test(tm, HIGHER_BUILD_VERSION)

    shell.wait_for_prompt(timeout=3)
    tm.check_with_mcumgr_command(HIGHER_BUILD_VERSION)
    logger.info("PASSED: Image with higher build number accepted.")
