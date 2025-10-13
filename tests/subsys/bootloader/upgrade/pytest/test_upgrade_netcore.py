# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Tests for MCUboot application and network core upgrade/downgrade scenarios."""

from __future__ import annotations

import logging

import pytest
from required_build import get_required_images_to_update
from twister_harness import DeviceAdapter, MCUmgr, Shell
from upgrade_test_manager import UpgradeTestWithMCUmgr

logger = logging.getLogger(__name__)

pytestmark = [
    pytest.mark.add_markers_if(
        '"nsib" in device_config.build_dir.name',
        [pytest.mark.nightly, pytest.mark.weekly],
    )
]


def test_upgrade_with_netcore(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that the application and network core are updated.

    APP based on smp_svr, MCUboot is the primary bootloader.
        Network core is enabled.
    Upload the network core image with a higher version,
        upload the APP image signed with higher version.
    Verify that the network core image and application are updated.

    (*) works also when NSIB is used as a primary bootloader
    and MCUboot is a second stage bootloader.
    """
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.increase_version()

    updated_app, updated_netcore, _ = get_required_images_to_update(
        dut, sign_version=tm.get_current_sign_version(), netcore_name=tm.build_params.net_core_name, firmware_version=3
    )

    tm.upload_images(updated_app, updated_netcore)
    tm.confirm_app_and_netcore()
    tm.reset_device()
    tm.verify_after_reset(
        lines=[
            "Image index: 0, Swap type: perm",
            "Image index: 1, Swap type: perm",
            "Image 0 copying the secondary slot",
            "Image 1 copying the secondary slot",
        ],
        no_lines=["Swap type: none", "insufficient version in secondary slot"],
    )
    tm.check_with_shell_command()
    logger.info("APP and NETCORE upgraded successfully")


def test_sw_downgrade_prevention_with_netcore(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that the application and network core are not downgraded.

    APP based on smp_svr, MCUboot is the primary bootloader.
        Network core is enabled.
    Upload the network core image with a lower version,
        upload the APP image signed with lower version.
    Verify that the network core image and application are not downgraded.

    (*) works also when NSIB is used as a primary bootloader
    and MCUboot is a second stage bootloader.
    """
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.decrease_version()

    updated_app, updated_netcore, _ = get_required_images_to_update(
        dut, sign_version=tm.get_current_sign_version(), firmware_version=1, netcore_name=tm.build_params.net_core_name
    )

    tm.upload_images(updated_app, updated_netcore)
    tm.confirm_app_and_netcore()
    tm.reset_device()
    tm.verify_after_reset(
        lines=[
            "Image index: 0, Swap type: perm",
            "Image index: 1, Swap type: perm",
            "insufficient version in secondary slot",
        ],
        no_lines=["copying the secondary slot"],
    )
    logger.info("Verify the initial APP is booted")
    tm.check_with_shell_command(tm.origin_mcuboot_version)
    logger.info("APP and NETCORE not downgraded")
