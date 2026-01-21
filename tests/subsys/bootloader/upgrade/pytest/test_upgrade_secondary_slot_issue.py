# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Test for upgrade scenario with unusable secondary slot in MCUboot."""

from __future__ import annotations

import logging
from pathlib import Path

from required_build import get_required_build
from twister_harness import DeviceAdapter, MCUmgr, Shell
from upgrade_test_manager import UpgradeTestWithMCUmgr

logger = logging.getLogger(__name__)


def test_upgrade_with_unusable_secondary_slot(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that the application is not upgraded to the secondary slot if it is not usable.

    Check if unusable secondary slot is removed after reboot and if next
    upgrade is executed successfully.
    """
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.welcome_str = "smp_sample: build time:"

    req_build_dir = get_required_build(
        dut,
        testsuite="mcuboot.upgrade.basic",
        suffix="_v2",
        extra_args='-- -DCONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION=\\"2.0.0+0\\"',
    )
    image_without_b0: Path = req_build_dir / "ref_smp_svr" / "zephyr" / "zephyr.signed.bin"

    tm.image_upload(image_without_b0)
    tm.image_test()

    tm.clear_buffer()
    tm.reset_device_from_shell()

    tm.verify_after_reset(
        lines=[
            "Cleaned-up secondary slot",
        ],
        no_lines=[
            "Starting swap using",
        ],
    )
    tm.check_with_shell_command()
    image_list = tm.mcumgr.get_image_list()
    assert len(image_list) == 1, "Secondary slot not removed"

    logger.info("Upgrade the application with a valid image")
    tm.increase_version()
    updated_app = tm.generate_image()
    tm.run_upgrade(updated_app)
    tm.verify_swap_test_after_reset()
