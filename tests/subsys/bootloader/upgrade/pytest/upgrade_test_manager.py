# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Test manager classes and utilities for MCUboot upgrade and downgrade scenarios."""

from __future__ import annotations

import functools
import logging
from abc import ABC, abstractmethod
from pathlib import Path

from helpers import retry, timer
from imgtool_wrapper import imgtool_sign
from parameters import BuildParameters
from twister_harness import DeviceAdapter, MCUmgr, Shell
from twister_harness.helpers.shell import ShellMCUbootCommandParsed
from twister_harness.helpers.utils import match_lines, match_no_lines

logger = logging.getLogger(__name__)


def use_serial(func):
    """Handle serial port connection for MCUmgr communication.

    Decorator to take care of serial port connection. This is required by MCUmgr,
    because it uses serial port to communicate with the device.
    """

    @functools.wraps(func)
    def wrapper(self, *args, **kwargs):
        reconnect = False
        if self.dut.is_device_connected():
            self.dut.disconnect()
            reconnect = True
        ret = func(self, *args, **kwargs)
        if reconnect:
            self.dut.connect()
        return ret

    return wrapper


def dut_connected(func):
    """Ensure DUT serial is connected before function call."""

    @functools.wraps(func)
    def wrapper(self, *args, **kwargs):
        if not self.dut.is_device_connected():
            self.dut.connect()
        ret = func(self, *args, **kwargs)
        return ret

    return wrapper


class UpgradeTestManagerException(Exception):
    """Base class for Upgrade Test Manager exceptions."""


class UpgradeTestManager(ABC):
    """Base class for Upgrade Test Manager."""

    welcome_str: str = "Jumping to the.*image slot"

    def __init__(self, dut: DeviceAdapter, shell: Shell):
        """Initialize UpgradeTestManager with DUT and shell."""
        self.dut = dut
        self.shell = shell
        self.build_params = BuildParameters.create_from_ncs_build_dir(
            dut.device_config.build_dir, dut.device_config.app_build_dir
        )
        self.origin_mcuboot_version: str = self.get_current_sign_version()
        self.shell.wait_for_prompt(timeout=10)

    @abstractmethod
    def reset_device(self):
        """Reset DUT."""

    @abstractmethod
    def image_upload(self, image: Path):
        """Upload image."""

    @dut_connected
    def clear_buffer(self):
        """Clear DUT buffer."""
        self.dut.clear_buffer()

    @dut_connected
    def reset_device_from_shell(self):
        """Reset DUT using shell command."""
        command = "kernel reboot cold"
        logger.debug(f"Reset device from shell: {command}")
        self.dut.write(f"{command}\n".encode())

    @dut_connected
    def request_upgrade_from_shell(self, confirm: bool = False):
        """Request MCUboot upgrade from shell."""
        command = "mcuboot request_upgrade"
        if confirm:
            command += " permanent"
        logger.debug(f"Request upgrade from shell: {command}")
        self.dut.write(f"{command}\n".encode())

    @dut_connected
    def _get_mcuboot_command_output(self) -> ShellMCUbootCommandParsed:
        """Get MCUboot command output from shell."""
        return ShellMCUbootCommandParsed.create_from_cmd_output(self.shell.exec_command("mcuboot"))

    @retry(1)
    def check_with_shell_command(
        self, version: str | None = None, slot: int = 0, swap_type: str | None = None
    ) -> None:
        """Check MCUboot state with shell command."""
        assert self.shell.wait_for_prompt(timeout=10), "No shell prompt"
        version = version or self.get_current_sign_version()
        mcuboot_areas = self._get_mcuboot_command_output()
        if "+" not in version:
            version = version + "+0"
        assert mcuboot_areas.areas[slot].version == version
        if swap_type:
            assert mcuboot_areas.areas[slot].swap_type == swap_type

    def increase_version(self):
        """Increase firmware version in build parameters."""
        self.build_params.imgtool_params.increase_version()
        logger.debug(f"Version increased to {self.get_current_sign_version()}")

    def get_current_sign_version(self) -> str:
        """Get current MCUboot imgtool sign version from build parameters."""
        return self.build_params.imgtool_params.version

    def get_version_string_to_verify_in_bootlog(self, version: str | None = None) -> str:
        """Get version string to verify in boot log."""
        if not version:
            version = self.get_current_sign_version()
        return f"Image version: v{version.split('+')[0]}"

    def decrease_version(self):
        """Set firmware version to lowest possible value."""
        assert self.origin_mcuboot_version != "0.0.0+0"
        self.build_params.imgtool_params.version = "0.0.0+0"
        logger.debug(f"Version set to {self.get_current_sign_version()} (lower than original)")

    def set_hw_counter_value(self, value: int) -> None:
        """Set hardware counter value in build parameters."""
        self.build_params.imgtool_params.security_counter = value
        logger.debug(f"Hardware counter value set to {value}")

    def generate_image(self, app_to_sign: Path | None = None, confirmed: bool = False) -> Path:
        """Sign application image with current version."""
        logger.info(f"Sign app with version {self.get_current_sign_version()}")
        if not app_to_sign:
            app_to_sign = self.build_params.app_to_sign
        updated_app = imgtool_sign(
            app_to_sign,
            self.build_params.imgtool_params,
            extra_args=["--confirm"] if confirmed else None,
        )
        assert updated_app.is_file()
        return updated_app

    def generate_netcore_image(self) -> Path:
        """Sign netcore app image with version from build parameters.

        Use with caution, because firmware version is not changed
        (app must be rebuilt with changed config). If you need changed
        firmware version (e.g. to test downgrade prevention),
        use methods from required_build.py.
        """
        self.build_params.update_params_for_netcore()
        netcore_image = self.generate_image(app_to_sign=self.build_params.netcore_to_sign)
        return netcore_image

    @timer
    def verify_after_reset(
        self, lines: list[str] | None = None, no_lines: list[str] | None = None
    ) -> None:
        """Verify device output after reset."""
        regex = f"{self.welcome_str}|Unable to find bootable image"
        output = self.dut.readlines_until(regex=regex, timeout=120)
        no_lines = no_lines or []
        match_no_lines(output, no_lines + ["Unable to find bootable image"])
        if lines:
            match_lines(output, lines)

    def verify_swap_in_boot_log(self, swap_type: str = "none", version: str | None = None):
        """Verify swap type and version in boot log after reset."""
        self.verify_after_reset(
            lines=[
                f"Swap type: {swap_type}",
                "Starting swap using",
                self.get_version_string_to_verify_in_bootlog(version),
            ]
        )

    def verify_swap_test_after_reset(self):
        """Verify test swap after reset."""
        logger.info("Verify new test APP is booted")
        self.verify_swap_in_boot_log(swap_type="test")

    def verify_swap_confirmed_after_reset(self):
        """Verify confirmed swap after reset."""
        logger.info("Verify new confirmed APP is booted")
        self.verify_swap_in_boot_log(swap_type="perm")

    def verify_no_swap_after_reset(self):
        """Verify that no swap occurred after reset."""
        self.verify_after_reset(
            lines=["Swap type: none", self.get_version_string_to_verify_in_bootlog()],
            no_lines=["Starting swap using"],
        )
        logger.info("Verify new APP is still booted")
        self.check_with_shell_command()


class UpgradeTestWithMCUmgr(UpgradeTestManager):
    """Upgrade test manager using MCUmgr for image operations."""

    def __init__(self, dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
        """Initialize UpgradeTestWithMCUmgr with DUT, shell, and MCUmgr."""
        super().__init__(dut, shell)
        self.mcumgr = mcumgr

    @use_serial
    @retry(1)
    def reset_device(self):
        """Reset device using MCUmgr."""
        self.mcumgr.reset_device()

    @use_serial
    @retry(1)
    @timer
    def image_upload(self, image: Path, slot: int | None = None):
        """Upload image to device using MCUmgr."""
        logger.info("Upload image with mcumgr{}".format(f" to slot {slot}" if slot else ""))
        image_with_slot = f"{image} -e -n {slot}" if slot is not None else image
        self.mcumgr.image_upload(image_with_slot)

    @use_serial
    def image_test(self, hash: str | None = None) -> None:
        """Test uploaded image using MCUmgr."""
        logger.info("Test uploaded APP image")
        self.mcumgr.image_test(hash)

    @use_serial
    def image_confirm(self, hash: str | None = None) -> None:
        """Confirm uploaded image using MCUmgr."""
        logger.info("Confirm uploaded APP image")
        self.mcumgr.image_confirm(hash)

    @use_serial
    def upload_images(
        self, app_image: Path, netcore_image: Path | None = None, app_external: Path | None = None
    ) -> int:
        """Upload multiple images to device."""
        self.image_upload(app_image, slot=0)
        slot_num = 1
        uploaded = 1
        if self.build_params.net_core_name:
            if netcore_image:
                self.image_upload(netcore_image, slot=1)
                uploaded += 1
            slot_num += 1
        if app_external:
            self.image_upload(app_external, slot=slot_num)
            uploaded += 1
        return uploaded

    @use_serial
    def mark_images(self, confirm=False) -> int:
        """Mark uploaded images as test or confirm using MCUmgr."""
        image_list = self.mcumgr.get_image_list()
        marked_images = 0
        for image in image_list:
            if "active" not in image.flags:
                if confirm:
                    self.mcumgr.image_confirm(image.hash)
                else:
                    self.mcumgr.image_test(image.hash)
                marked_images += 1
        return marked_images

    @use_serial
    def check_with_mcumgr_command(self, version: str) -> None:
        """Check active image version using MCUmgr."""
        image_list = self.mcumgr.get_image_list()
        active_image = None
        for image in image_list:
            if "active" in image.flags:
                active_image = image
                break
        else:
            raise UpgradeTestManagerException("No active image found")
        # version displayed by MCUmgr does not print +0 and changes + to '.' for non-zero values
        assert active_image.version == version.replace("+0", "").replace("+", ".")  # type: ignore

    @use_serial
    def run_upgrade(self, image: Path, confirm: bool = False) -> None:
        """Upload image, test or confirm it, and reset device using MCUmgr."""
        self.image_upload(image)
        if confirm:
            self.mcumgr.image_confirm()
        else:
            self.mcumgr.image_test()
        self.reset_device()

    @use_serial
    def confirm_app_and_netcore(self):
        """Confirm uploaded APP and NETCORE images using MCUmgr."""
        logger.info("Confirm uploaded APP and NETCORE images")
        image_list = self.mcumgr.get_image_list()
        assert len(image_list) > 2
        # TODO: strange behavior when testing with NSIB,
        # if APP image is mark `test` or `confirm` first, than NETCORE
        # image is not updated after reboot (must be marked once again and rebooted)
        self.mcumgr.image_confirm(image_list[2].hash)
        self.mcumgr.image_confirm(image_list[1].hash)
