# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Tests for Direct XIP upgrade, revert, and downgrade prevention scenarios with MCUboot."""

from __future__ import annotations

import logging
from pathlib import Path

import pytest
from required_build import RequiredBuild
from twister_harness import DeviceAdapter, MCUmgr, Shell
from twister_harness.helpers.utils import find_in_config
from upgrade_test_manager import UpgradeTestWithMCUmgr

logger = logging.getLogger(__name__)


class UpgradeTestDirectXipUseMCUmgr(UpgradeTestWithMCUmgr):
    """Test manager for Direct XIP upgrade scenarios using MCUmgr."""

    def generate_image_for_direct_xip_secondary_slot(self) -> Path:
        """Generate image for direct XIP secondary slot."""
        self.build_params.imgtool_params.rom_fixed = find_in_config(
            self.build_params.pm_config, "PM_MCUBOOT_SECONDARY_ADDRESS"
        )
        logger.info("Generate image for direct xip secondary slot")
        return self.generate_image(app_to_sign=self.build_params.mcuboot_secondary_app_to_sign)

    def generate_image_for_direct_xip_primary_slot(self) -> Path:
        """Generate image for direct XIP primary slot."""
        self.build_params.imgtool_params.rom_fixed = find_in_config(
            self.build_params.pm_config, "PM_MCUBOOT_PRIMARY_ADDRESS"
        )
        logger.info("Generate image for direct xip primary slot")
        return self.generate_image(app_to_sign=self.build_params.app_to_sign)

    def verify_direct_xip_secondary_slot_loaded(self, version: str | None = None):
        """Verify that the secondary slot is loaded in Direct XIP mode."""
        version = version or self.get_current_sign_version()
        self.verify_after_reset(
            lines=[
                "Starting Direct-XIP bootloader",
                f"Secondary slot: version={version}",
                "Image 0 loaded from the secondary slot",
            ]
        )
        logger.info("Verify new APP is booted")
        self.check_with_shell_command(version, slot=1)

    def verify_direct_xip_primary_slot_loaded(self, version: str | None = None):
        """Verify that the primary slot is loaded in Direct XIP mode."""
        version = version or self.get_current_sign_version()
        self.verify_after_reset(
            lines=[
                "Starting Direct-XIP bootloader",
                f"Primary   slot: version={version}",
                "Image 0 loaded from the primary slot",
            ]
        )
        logger.info("Verify new APP is booted")
        self.check_with_shell_command(version, slot=0)


def get_required_build_for_direct_xip_nRF54H(dut: DeviceAdapter, sign_version: str) -> Path:
    """Get required images to update for direct XIP on nRF54H."""
    extra_args = " --"
    extra_args += f' -DCONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION=\\"{sign_version}\\"'

    req_build = RequiredBuild.create_from_dut(
        dut=dut,
        suffix="_" + sign_version.replace(".", "_").replace("+", "_"),
        extra_args=extra_args,
    )
    return req_build.get_ready_build()


class UpgradeTestDirectXipUseMCUmgrNRF54H(UpgradeTestDirectXipUseMCUmgr):
    """Test manager for Direct XIP upgrade scenarios for nRF54H."""

    def generate_image_for_direct_xip_secondary_slot(self) -> Path:
        """Generate image for direct XIP secondary slot."""
        logger.info("Generate image for direct xip secondary slot")
        req_build_dir = get_required_build_for_direct_xip_nRF54H(
            self.dut, self.get_current_sign_version()
        )
        secondary_image = req_build_dir / "zephyr_secondary_app.signed.bin"
        assert secondary_image.is_file(), f"Secondary image not found: {secondary_image}"
        return secondary_image

    def generate_image_for_direct_xip_primary_slot(self) -> Path:
        """Generate image for direct XIP primary slot."""
        logger.info("Generate image for direct xip primary slot")
        req_build_dir = get_required_build_for_direct_xip_nRF54H(
            self.dut, self.get_current_sign_version()
        )
        primary_image = req_build_dir / "zephyr.signed.bin"
        assert primary_image.is_file(), f"Primary image not found: {primary_image}"
        return primary_image


def factory_upgrade_test_direct_xip(
    dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr
) -> UpgradeTestDirectXipUseMCUmgr:
    """Create an instance of UpgradeTestDirectXipUseMCUmgr."""
    if "nrf54h" in dut.device_config.platform:
        return UpgradeTestDirectXipUseMCUmgrNRF54H(dut, shell, mcumgr)
    return UpgradeTestDirectXipUseMCUmgr(dut, shell, mcumgr)


class TestDirectXipWithRevert:
    """Test Direct XIP upgrade and revert scenarios."""

    @pytest.mark.nightly
    def test_direct_xip_upgrade_with_confirm(
        self, dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr
    ):
        """Verify that the application can be updated and confirmed.

        APP based on smp_svr, MCUboot is the primary bootloader.
        Direct XIP with revert mode is enabled.
        Upload the application image signed with a higher version.
        Test image. Reset DUT.
        Verify that the updated application is booted.
        Confirm image. Reset DUT.
        Verify that the new application is still booted.
        """
        tm = factory_upgrade_test_direct_xip(dut, shell, mcumgr)
        tm.increase_version()
        updated_app = tm.generate_image_for_direct_xip_secondary_slot()
        tm.run_upgrade(updated_app)
        tm.verify_direct_xip_secondary_slot_loaded()

        tm.image_confirm()
        tm.reset_device()
        # still loaded from the secondary slot
        tm.verify_direct_xip_secondary_slot_loaded()

    def test_direct_xip_upgrade_with_revert(self, dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
        """Verify that the application is reverted if not confirmed.

        APP based on smp_svr, MCUboot is the primary bootloader.
        Direct XIP with revert mode is enabled.
        Upload the application image signed with a higher version.
        Test image. Reset DUT.
        Verify that the updated application is booted.
        Reset DUT without confirming the image.
        Verify that the application is reverted.
        """
        tm = factory_upgrade_test_direct_xip(dut, shell, mcumgr)
        tm.increase_version()
        updated_app = tm.generate_image_for_direct_xip_secondary_slot()
        tm.run_upgrade(updated_app)
        tm.verify_direct_xip_secondary_slot_loaded()

        logger.info("Revert images")
        tm.reset_device()
        # loaded original app from the primary slot
        tm.verify_direct_xip_primary_slot_loaded(version=tm.origin_mcuboot_version)

    @pytest.mark.nightly
    def test_direct_xip_upgrade_multiple(self, dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
        """Verify that the application can be updated multiple times.

        APP based on smp_svr, MCUboot is the primary bootloader.
        Direct XIP with revert mode is enabled.
        Upload the second app image signed with a higher version.
        Confirm image. Reset DUT.
        Upload the third app signed with higher version (now to slot 0, use app_update.bin)
        Confirm image. Reset DUT.
        Verify that the third application is booted.
        """
        tm = factory_upgrade_test_direct_xip(dut, shell, mcumgr)
        tm.increase_version()
        updated_app = tm.generate_image_for_direct_xip_secondary_slot()
        tm.run_upgrade(updated_app, confirm=True)
        tm.verify_direct_xip_secondary_slot_loaded()

        # sign the third app and upload them to the primary slot
        tm.increase_version()
        third_app = tm.generate_image_for_direct_xip_primary_slot()
        tm.run_upgrade(third_app, confirm=True)
        tm.verify_direct_xip_primary_slot_loaded()

    def test_direct_xip_downgrade_prevention(
        self, dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr
    ):
        """Verify that the application is not downgraded.

        APP based on smp_svr, MCUboot is the primary bootloader.
        Direct XIP mode is enabled.
        Upload the application image signed with a lower version.
        Test image. Reset DUT.
        Verify that the application is not downgraded.
        """
        tm = factory_upgrade_test_direct_xip(dut, shell, mcumgr)
        tm.decrease_version()
        updated_app = tm.generate_image_for_direct_xip_secondary_slot()
        tm.run_upgrade(updated_app)

        # not downgraded, loaded original app from the primary slot
        tm.verify_direct_xip_primary_slot_loaded(version=tm.origin_mcuboot_version)


class TestDirectXip:
    """Test Direct XIP upgrade and downgrade prevention without revert mode."""

    def test_direct_xip_no_revert_and_downgrade_prev(
        self, dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr
    ):
        """Verify that the application can be updated from both slots and not downgraded with lower
        version.

        APP based on smp_svr, MCUboot is the primary bootloader.
        Direct XIP mode is enabled.
        Upload the application image signed with a lower version.
        Test image. Reset DUT.
        Verify that the application is not downgraded.
        """
        tm = factory_upgrade_test_direct_xip(dut, shell, mcumgr)
        tm.decrease_version()
        downgraded_app = tm.generate_image_for_direct_xip_secondary_slot()
        tm.image_upload(downgraded_app)
        tm.reset_device_from_shell()
        tm.verify_direct_xip_primary_slot_loaded(version=tm.origin_mcuboot_version)
        logger.info("Not downgraded from secondary slot")

        tm.increase_version()
        tm.increase_version()
        updated_app = tm.generate_image_for_direct_xip_secondary_slot()
        tm.image_upload(updated_app)
        tm.reset_device_from_shell()
        tm.verify_direct_xip_secondary_slot_loaded()
        logger.info("Upgraded from secondary slot")

        # sign the third app and upload them to the primary slot
        tm.increase_version()
        third_app = tm.generate_image_for_direct_xip_primary_slot()
        tm.image_upload(third_app)
        tm.reset_device_from_shell()
        tm.verify_direct_xip_primary_slot_loaded()
        logger.info("Upgraded from primary slot")

        # not downgraded, upload the original app to the secondary slot,
        # and verify that the primary slot is still loaded
        if "nrf54h" in dut.device_config.platform:
            tm.image_upload(dut.device_config.build_dir / "zephyr_secondary_app.signed.bin")
        else:
            tm.image_upload(
                dut.device_config.build_dir
                / "mcuboot_secondary_app"
                / "zephyr"
                / "zephyr.signed.bin"
            )
        tm.reset_device_from_shell()
        tm.verify_direct_xip_primary_slot_loaded()
        logger.info("Not downgraded from secondary slot")
