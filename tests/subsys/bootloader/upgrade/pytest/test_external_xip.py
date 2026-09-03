# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Tests for external XIP and Direct XIP upgrade/downgrade scenarios with MCUboot."""

from __future__ import annotations

import logging
import time
from contextlib import contextmanager
from pathlib import Path

from packaging.version import Version
from twister_harness import DeviceAdapter, MCUmgr, Shell
from twister_harness.helpers.utils import find_in_config
from twister_harness_ext.utils.common import reset_board
from twister_harness_ext.utils.dts_helper import (
    get_partition_address,
    get_partition_size,
)
from upgrade_test_manager import UpgradeTestWithMCUmgr

logger = logging.getLogger(__name__)

SEC_APP_DIR_NAME = "extxip_smp_svr_slot1_variant"


class UpgradeTestExtXipUseMCUmgr(UpgradeTestWithMCUmgr):
    """Upgrade test manager for external XIP using MCUmgr."""

    welcome_str = "smp_sample: build time:"

    def __init__(self, dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
        """Initialize UpgradeTestExtXipUseMCUmgr with DUT, shell, and MCUmgr."""
        super().__init__(dut, shell, mcumgr)
        self.app_to_sign_int = self.build_params.app_build_dir / "zephyr" / "zephyr.internal.hex"
        self.app_to_sign_ext = self.build_params.app_build_dir / "zephyr" / "zephyr.external.hex"
        self.slot_size_int = self.build_params.imgtool_params.slot_size
        self.mcuboot_edt = self.build_params.build_dir / "mcuboot" / "zephyr" / "edt.pickle"
        self.qspi_xip_image_number = int(
            find_in_config(
                self.build_params.build_dir / "mcuboot" / "zephyr" / ".config",
                "CONFIG_MCUBOOT_QSPI_XIP_IMAGE_NUMBER",
            )
        )
        self.slot_size_ext = hex(
            get_partition_size(self.mcuboot_edt, self._slot_label(primary=True))
        )

    def _slot_label(self, primary: bool, image_number: int | None = None) -> str:
        """Return the DTS slot partition label for a given MCUboot image.

        MCUboot maps image ``N`` to ``slot{2N}_partition`` (primary) and
        ``slot{2N+1}_partition`` (secondary). The internal application is
        image 0; the external QSPI XIP application is ``qspi_xip_image_number``.
        """
        image_number = self.qspi_xip_image_number if image_number is None else image_number
        slot = 2 * image_number + (0 if primary else 1)
        return f"slot{slot}_partition"

    @contextmanager
    def _pad_header_enabled(self):
        """Temporarily force imgtool ``--pad-header`` while signing an image.

        Only the internal ``zephyr.internal.hex`` reserves the MCUboot header
        area (it starts with zeros); the external XIP image and the netcore
        image start directly with code, so imgtool must prepend the header via
        ``--pad-header``. With Partition Manager this was covered globally by
        ``pad_header = pm or tfm``; now it is requested explicitly for those
        images that need it.
        """
        original_pad_header = self.build_params.imgtool_params.pad_header
        self.build_params.imgtool_params.pad_header = True
        try:
            yield
        finally:
            self.build_params.imgtool_params.pad_header = original_pad_header

    def generate_external_image(self, app_to_sign: Path) -> Path:
        """Sign an external XIP image with header padding enabled."""
        with self._pad_header_enabled():
            return self.generate_image(app_to_sign)

    def generate_netcore_image(self) -> Path:
        """Sign the netcore image with header padding enabled."""
        with self._pad_header_enabled():
            return super().generate_netcore_image()

    def generate_app_images(self) -> tuple[Path, Path]:
        """Generate internal and external app images for XIP."""
        self.build_params.imgtool_params.slot_size = self.slot_size_int
        int_app = self.generate_image(self.app_to_sign_int)

        self.build_params.imgtool_params.slot_size = self.slot_size_ext
        ext_app = self.generate_external_image(self.app_to_sign_ext)
        return (int_app, ext_app)


def test_upgrade_extxip(
    dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr, required_build_dirs: list[str]
):
    """Verify that the application and the netcore (if configured) can be updated.

    When external XIP is used and is not reverted after reset
    because of MCUBOOT_MODE_OVERWRITE_ONLY flag.
    """
    dut.device_config.build_dir = Path(required_build_dirs[0])
    tm = UpgradeTestExtXipUseMCUmgr(dut, shell, mcumgr)
    tm.increase_version()
    int_app, ext_app = tm.generate_app_images()
    netcore_image = None
    if tm.build_params.net_core_name:
        netcore_image = tm.generate_netcore_image()
    num_of_images = tm.upload_images(int_app, netcore_image, ext_app)
    assert tm.mark_images() == num_of_images
    logger.info(f"Verify that {num_of_images} images are updated")
    tm.clear_buffer()
    reset_board(dut.device_config.id)
    tm.verify_after_reset(
        lines=[f"Image index: {i}, Swap type: test" for i in range(num_of_images)]
        + [f"Image {i} copying the secondary slot" for i in range(num_of_images)]
    )

    time.sleep(2)  # wait for reset messages from the device
    logger.info("Reset once again and verify that the images are not reverted")
    tm.clear_buffer()
    reset_board(dut.device_config.id)
    current_version = Version(tm.get_current_sign_version()).base_version
    tm.verify_after_reset(
        lines=[f"Image index: {i}, Swap type: none" for i in range(num_of_images)]
        + [f"Image version: v{current_version}"],
        no_lines=["swap using move algorithm", "copying the secondary slot"],
    )


def test_sw_downgrade_prevention_extxip(
    dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr, required_build_dirs: list[str]
):
    """Verify that the application is not downgraded when external XIP is used."""
    dut.device_config.build_dir = Path(required_build_dirs[0])
    tm = UpgradeTestExtXipUseMCUmgr(dut, shell, mcumgr)
    tm.decrease_version()
    int_app, ext_app = tm.generate_app_images()
    num_of_images = tm.upload_images(int_app, app_external=ext_app)
    assert tm.mark_images(confirm=True) == num_of_images
    logger.info("Verify that the application is not downgraded")
    tm.clear_buffer()
    reset_board(dut.device_config.id)

    ext_index = 1 if not tm.build_params.net_core_name else 2
    origin_version = Version(tm.origin_mcuboot_version).base_version
    tm.verify_after_reset(
        lines=[
            "Image index: 0, Swap type: perm",
            f"Image index: {ext_index}, Swap type: perm",
            "Insufficient version in secondary slot",
            f"Image version: v{origin_version}",
        ],
        no_lines=["copying the secondary slot to the primary slot", "swap using move algorithm"],
    )


class UpgradeTestExtXipWithDirectXip(UpgradeTestExtXipUseMCUmgr):
    """Upgrade test manager for external XIP with Direct XIP support."""

    def __init__(self, dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
        """Initialize UpgradeTestExtXipWithDirectXip with DUT, shell, and MCUmgr."""
        super().__init__(dut, shell, mcumgr)
        self.mcuboot_secondary_app_to_sign_int = (
            self.build_params.build_dir / SEC_APP_DIR_NAME / "zephyr" / "zephyr.internal.hex"
        )
        self.mcuboot_secondary_app_to_sign_ext = (
            self.build_params.build_dir / SEC_APP_DIR_NAME / "zephyr" / "zephyr.external.hex"
        )
        self.rom_fixed_primary = hex(
            get_partition_address(
                self.mcuboot_edt, self._slot_label(primary=True, image_number=0), absolute=True
            )
        )
        self.rom_fixed_secondary = hex(
            get_partition_address(
                self.mcuboot_edt, self._slot_label(primary=False, image_number=0), absolute=True
            )
        )
        self.rom_fixed_primary_ext = hex(
            get_partition_address(self.mcuboot_edt, self._slot_label(primary=True), absolute=True)
        )
        self.rom_fixed_secondary_ext = hex(
            get_partition_address(self.mcuboot_edt, self._slot_label(primary=False), absolute=True)
        )

    def generate_app_images_for_direct_xip_secondary_slot(self) -> tuple[Path, Path]:
        """Generate app images for Direct XIP secondary slot."""
        self.build_params.imgtool_params.slot_size = self.slot_size_int
        self.build_params.imgtool_params.rom_fixed = self.rom_fixed_secondary
        int_app = self.generate_image(self.mcuboot_secondary_app_to_sign_int)

        self.build_params.imgtool_params.slot_size = self.slot_size_ext
        self.build_params.imgtool_params.rom_fixed = self.rom_fixed_secondary_ext
        ext_app = self.generate_external_image(self.mcuboot_secondary_app_to_sign_ext)
        return (int_app, ext_app)

    def generate_app_images_for_direct_xip_primary_slot(self) -> tuple[Path, Path]:
        """Generate app images for Direct XIP primary slot."""
        self.build_params.imgtool_params.slot_size = self.slot_size_int
        self.build_params.imgtool_params.rom_fixed = self.rom_fixed_primary
        int_app = self.generate_image(self.app_to_sign_int)

        self.build_params.imgtool_params.slot_size = self.slot_size_ext
        self.build_params.imgtool_params.rom_fixed = self.rom_fixed_primary_ext
        ext_app = self.generate_external_image(self.app_to_sign_ext)
        return (int_app, ext_app)

    def verify_extxip_direct_xip_secondary_slot_loaded(self, version: str | None = None):
        """Verify that Direct XIP secondary slot is loaded after reset."""
        version = version or self.get_current_sign_version()
        self.verify_after_reset(
            lines=[
                "Starting Direct-XIP bootloader",
                f"Secondary slot: version={version}",
                "Image 0 loaded from the secondary slot",
                "Image 1 loaded from the secondary slot",
                f"Image version: v{Version(version).base_version}",
            ]
        )
        logger.info("Verify new APP is booted")

    def verify_extxip_direct_xip_primary_slot_loaded(self, version: str | None = None):
        """Verify that Direct XIP primary slot is loaded after reset."""
        version = version or self.get_current_sign_version()
        self.verify_after_reset(
            lines=[
                "Starting Direct-XIP bootloader",
                f"slot: version={version}",
                f"Primary slot: version={version}",
                "Image 0 loaded from the primary slot",
                "Image 1 loaded from the primary slot",
                f"Image version: v{Version(version).base_version}",
            ],
            no_lines=[f"Secondary slot: version={version}"],
        )
        logger.info("Verify new APP is booted")


def test_upgrade_extxip_with_direct_xip(
    dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr, required_build_dirs: list[str]
):
    """Verify that the application can be updated when external XIP is used with Direct XIP."""
    dut.device_config.build_dir = Path(required_build_dirs[0])
    tm = UpgradeTestExtXipWithDirectXip(dut, shell, mcumgr)
    tm.decrease_version()
    int_app, ext_app = tm.generate_app_images_for_direct_xip_secondary_slot()
    tm.upload_images(int_app, app_external=ext_app)
    reset_board(dut.device_config.id)
    tm.verify_extxip_direct_xip_primary_slot_loaded(version=tm.origin_mcuboot_version)
    logger.info("Not downgraded from secondary slot")

    tm.increase_version()
    tm.increase_version()
    int_app, ext_app = tm.generate_app_images_for_direct_xip_secondary_slot()
    tm.upload_images(int_app, app_external=ext_app)
    reset_board(dut.device_config.id)
    tm.verify_extxip_direct_xip_secondary_slot_loaded()
    logger.info("Upgraded from secondary slot")

    # sign the third app and upload them to the primary slot
    tm.increase_version()
    int_app, ext_app = tm.generate_app_images_for_direct_xip_primary_slot()
    tm.upload_images(int_app, app_external=ext_app)
    reset_board(dut.device_config.id)
    tm.verify_extxip_direct_xip_primary_slot_loaded()
    logger.info("Upgraded from primary slot")

    # not downgraded, upload the original app to the secondary slot,
    # and verify that the primary slot is still loaded
    tm.upload_images(
        app_image=dut.device_config.build_dir
        / SEC_APP_DIR_NAME
        / "zephyr"
        / "zephyr.internal.signed.bin",
        app_external=dut.device_config.build_dir
        / SEC_APP_DIR_NAME
        / "zephyr"
        / "zephyr.external.signed.bin",
    )
    reset_board(dut.device_config.id)
    tm.verify_extxip_direct_xip_primary_slot_loaded()
    logger.info("Not downgraded from secondary slot")
