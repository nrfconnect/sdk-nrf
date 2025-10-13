# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Test for upgrade scenario where the application is damaged and recovered using serial recovery."""

from __future__ import annotations

import logging
import time

from helpers import nrfutil_write, reset_board, retry
from twister_harness import DeviceAdapter, MCUmgr, Shell
from twister_harness.helpers.utils import find_in_config
from upgrade_test_manager import UpgradeTestWithMCUmgr

logger = logging.getLogger(__name__)


@retry(2)
def get_image_list(mcumgr: MCUmgr):
    """Get the list of images from the device using mcumgr."""
    images = mcumgr.get_image_list()
    if not images:
        logger.debug("No images found")
    else:
        logger.debug(f"Found image with version {images[0].version}, hash: {images[0].hash}")
    return images


def test_serial_recovery_after_damaging_app(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that the application is recovered with serial recovery after no bootable app was found."""
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    # change the welcome string, because logs from MCUboot are disabled
    tm.welcome_str = "smp_sample: build time:"
    dut.disconnect()

    logger.info("Damage the application by writing invalid data to the beginning of the image")
    app_address = find_in_config(dut.device_config.build_dir / "pm.config", "PM_MCUBOOT_PRIMARY_ADDRESS")
    nrfutil_write(str(app_address), "0xdeadbeef", dut.device_config.id)
    reset_board(dut.device_config.id)
    time.sleep(2)  # wait for the device to reset and enter serial recovery mode
    image_list = get_image_list(mcumgr)
    assert not image_list, "Image list should be empty in serial recovery mode, when no bootable app is present"

    tm.image_upload(dut.device_config.app_build_dir / "zephyr" / "zephyr.signed.bin")  # type: ignore
    image_list = get_image_list(mcumgr)
    assert len(image_list) == 1, "Image list should contain one image after upload"
    tm.reset_device()
    dut.connect()
    tm.verify_after_reset()
    tm.check_with_shell_command()
    logger.info("PASSED: Application is recovered after serial recovery")


def test_serial_recovery_after_pin_reset(no_reset, dut: DeviceAdapter, mcumgr: MCUmgr):  # noqa: ARG001
    """Verify that the application is recovered with serial recovery after pin reset."""
    # west flash uses RESET_PIN in some boards, reset here with nrfutil
    reset_board(dut.device_config.id)
    shell = Shell(dut)
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.welcome_str = "smp_sample: build time:"
    dut.disconnect()

    logger.info("Reset the application with RESET_PIN to enter serial recovery mode")
    reset_board(dut.device_config.id, reset_kind="RESET_PIN")
    time.sleep(2)  # wait for the device to reset and enter serial recovery mode
    image_list = get_image_list(mcumgr)
    assert image_list, "Image list should not be empty after pin reset"

    tm.increase_version()
    updated_app = tm.generate_image()
    tm.image_upload(updated_app)
    image_list = get_image_list(mcumgr)
    assert len(image_list) == 1, "Image list should contain one image after upload"
    reset_board(dut.device_config.id)
    dut.connect()
    tm.verify_after_reset()
    tm.check_with_shell_command()
    logger.info("PASSED: Application is recovered after serial recovery")
