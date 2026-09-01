# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Test for upgrade scenario with an encrypted secondary slot in MCUboot."""

from __future__ import annotations

import logging

from twister_harness import DeviceAdapter, MCUmgr, Shell
from upgrade_test_manager import UpgradeTestWithMCUmgr

logger = logging.getLogger(__name__)


def test_upgrade_with_encrypted_secondary_slot(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that an encrypted image in the secondary slot is swapped, not erased.

    When NSIB is the primary bootloader, the secondary slot is shared between the
    application and MCUboot itself, so MCUboot has to work out which primary slot
    an image found there belongs to. Encrypting the image must not prevent that
    assignment. If it does, a valid update is destroyed by the unusable secondary
    slot clean-up and no encrypted upgrade can ever complete.
    """
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.increase_version()
    updated_app = tm.generate_image()
    tm.run_upgrade(updated_app)

    tm.verify_after_reset(
        lines=[
            "Swap type: test",
            "Starting swap using",
            tm.get_version_string_to_verify_in_bootlog(),
        ],
        no_lines=[
            # sec_slot_cleanup_if_unusable() erasing the staged image
            "Erase secondary: img",
            # boot_validated_swap_type() discarding a slot it cannot place
            "Cleaned-up secondary slot",
        ],
    )
    tm.check_with_shell_command()

    logger.info("Verify that the encrypted image is reverted when it is not confirmed")
    tm.reset_device()
    tm.verify_swap_in_boot_log(swap_type="revert", version=tm.origin_mcuboot_version)
