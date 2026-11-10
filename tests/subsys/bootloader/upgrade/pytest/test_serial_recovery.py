# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Test for upgrade scenario where the application is damaged.
Application is recovered using serial recovery.
"""

from __future__ import annotations

import logging
import time

import pytest
from twister_harness import DeviceAdapter, MCUmgr, Shell
from twister_harness_ext.adapters.nrfutil import nrfutil_write, reset_board
from twister_harness_ext.utils.dts_helper import (
    get_code_partition_address,
    get_partition_address,
)
from twister_harness_ext.utils.helpers import retry
from upgrade_test_manager import UpgradeTestWithMCUmgr

logger = logging.getLogger(__name__)

APPLICATION_CONTEXT_LOG = "Now running: application"
RECOVERY_CONTEXT_LOG = "Now running: MCUboot serial recovery"


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
    """Verify that the app is recovered with serial recovery after no bootable app was found."""
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.welcome_str = APPLICATION_CONTEXT_LOG
    dut.disconnect()

    edt_data = dut.device_config.app_build_dir / "zephyr" / "edt.pickle"  # type: ignore
    app_address = get_code_partition_address(edt_data)
    secondary_address = get_partition_address(edt_data, "slot1_partition", absolute=True)
    logger.info(
        "Damage application headers at primary address %s and secondary address %s",
        app_address,
        secondary_address,
    )
    nrfutil_write(str(app_address), "0xdeadbeef", dut.device_config.id)
    nrfutil_write(str(secondary_address), "0xdeadbeef", dut.device_config.id)
    reset_board(dut.device_config.id)
    time.sleep(2)  # wait for the device to reset and enter serial recovery mode
    image_list = get_image_list(mcumgr)
    assert not image_list, (
        "Image list should be empty in serial recovery mode, when no bootable app is present"
    )
    logger.info(RECOVERY_CONTEXT_LOG)

    tm.image_upload(dut.device_config.app_build_dir / "zephyr" / "zephyr.signed.bin")  # type: ignore
    image_list = get_image_list(mcumgr)
    assert len(image_list) == 1, "Image list should contain one image after upload"
    tm.reset_device()
    dut.connect()
    tm.verify_after_reset()
    logger.info(APPLICATION_CONTEXT_LOG)
    tm.check_with_shell_command()
    logger.info("PASSED: Application is recovered after serial recovery")


def test_serial_recovery_after_pin_reset(no_reset, dut: DeviceAdapter, mcumgr: MCUmgr):  # noqa: ARG001
    """Verify that the application is recovered with serial recovery after pin reset."""
    # west flash uses RESET_PIN in some boards, reset here with nrfutil
    reset_board(dut.device_config.id)
    shell = Shell(dut)
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.welcome_str = APPLICATION_CONTEXT_LOG
    dut.disconnect()

    logger.info("Reset the application with RESET_PIN to enter serial recovery mode")
    reset_board(dut.device_config.id, reset_kind="RESET_PIN")
    time.sleep(2)  # wait for the device to reset and enter serial recovery mode
    image_list = get_image_list(mcumgr)
    assert image_list, "Image list should not be empty after pin reset"
    logger.info(RECOVERY_CONTEXT_LOG)

    tm.increase_version()
    updated_app = tm.generate_image()
    tm.image_upload(updated_app)
    image_list = get_image_list(mcumgr)
    assert len(image_list) == 1, "Image list should contain one image after upload"
    dut.connect()
    reset_board(dut.device_config.id)
    tm.verify_after_reset()
    logger.info(APPLICATION_CONTEXT_LOG)
    tm.check_with_shell_command()
    logger.info("PASSED: Application is recovered after serial recovery")


@pytest.mark.usefixtures("no_reset")
def test_serial_recovery_timeout(dut: DeviceAdapter, mcumgr: MCUmgr):
    """Verify that MCUboot leaves serial recovery when no MCUmgr command is received."""
    # west flash uses RESET_PIN on some boards; clear that reset reason first.
    reset_board(dut.device_config.id)
    dut.disconnect()

    logger.info("Enter serial recovery with RESET_PIN and wait for its timeout")
    reset_board(dut.device_config.id, reset_kind="RESET_PIN")
    logger.info(RECOVERY_CONTEXT_LOG)
    time.sleep(15)

    image_list = get_image_list(mcumgr)
    assert len(image_list) == 1, "Application image should be available after recovery timeout"
    assert "active" in image_list[0].flags, "Application should be active after recovery timeout"

    dut.connect()
    shell = Shell(dut)
    assert shell.wait_for_prompt(timeout=10), (
        "Application shell should be available after recovery timeout"
    )
    logger.info(APPLICATION_CONTEXT_LOG)


@pytest.mark.usefixtures("no_reset")
def test_direct_upload(dut: DeviceAdapter, mcumgr: MCUmgr):
    """Verify direct upload to a selected image slot."""
    dut.disconnect()
    reset_board(dut.device_config.id, reset_kind="RESET_PIN")
    time.sleep(2)

    signed_image = dut.device_config.app_build_dir / "zephyr" / "zephyr.signed.bin"  # type: ignore
    mcumgr.image_upload(signed_image, slot=2, timeout=60)
    images = get_image_list(mcumgr)
    assert any(image.image == 0 and image.slot == 1 for image in images)
    logger.info(RECOVERY_CONTEXT_LOG)


def is_application_running(mcumgr: MCUmgr) -> bool:
    """Check whether the application answers SMP echo, which MCUboot serial recovery does not."""
    return mcumgr.run_command("-t 3 echo ping").strip() == "ping"


def request_recovery_from_application(dut: DeviceAdapter, mcumgr: MCUmgr, command: str) -> None:
    """Request serial recovery with an application shell command and verify MCUboot runs."""
    logger.info("Request serial recovery from the application: %s", command)
    dut.write(f"{command}\n".encode())
    time.sleep(2)  # wait for the device to reset and enter serial recovery mode
    dut.disconnect()

    assert not is_application_running(mcumgr), "Application should not run after recovery request"
    assert get_image_list(mcumgr), "Image list should not be empty in serial recovery mode"
    logger.info(RECOVERY_CONTEXT_LOG)


def verify_one_shot_recovery_request(
    dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr, command: str
) -> None:
    """Request serial recovery from the application and verify it is consumed after one reset."""
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.welcome_str = APPLICATION_CONTEXT_LOG

    request_recovery_from_application(dut, mcumgr, command)

    tm.reset_device()
    dut.connect()
    tm.verify_after_reset()
    logger.info(APPLICATION_CONTEXT_LOG)

    logger.info("Reset again to verify that the recovery request was cleared")
    reset_board(dut.device_config.id)
    tm.verify_after_reset()
    dut.disconnect()
    assert is_application_running(mcumgr), "Application should run after the second reset"
    logger.info("PASSED: Serial recovery request is consumed after one reset")


def verify_update_after_recovery_request(
    dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr, command: str
) -> None:
    """Request serial recovery from the application and update the application through it."""
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.welcome_str = APPLICATION_CONTEXT_LOG

    request_recovery_from_application(dut, mcumgr, command)

    tm.increase_version()
    updated_app = tm.generate_image()
    tm.image_upload(updated_app)
    image_list = get_image_list(mcumgr)
    assert len(image_list) == 1, "Image list should contain one image after upload"
    tm.reset_device()
    dut.connect()
    tm.verify_after_reset()
    logger.info(APPLICATION_CONTEXT_LOG)
    tm.check_with_shell_command()
    logger.info("PASSED: Application is updated after serial recovery request")


def test_serial_recovery_boot_mode_request(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify one-shot serial recovery entry requested through the retained boot mode."""
    verify_one_shot_recovery_request(dut, shell, mcumgr, "mcuboot serial_recovery")


def test_serial_recovery_boot_req_request(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify one-shot serial recovery entry requested through the bootloader request subsystem."""
    verify_one_shot_recovery_request(dut, shell, mcumgr, "boot_request serial_recovery")


def test_serial_recovery_boot_mode_update(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify application update after serial recovery requested through the retained boot mode."""
    verify_update_after_recovery_request(dut, shell, mcumgr, "mcuboot serial_recovery")


def test_serial_recovery_boot_req_update(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify application update after serial recovery requested through the bootloader request."""
    verify_update_after_recovery_request(dut, shell, mcumgr, "boot_request serial_recovery")
