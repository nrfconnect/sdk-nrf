# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Tests for LZMA compression and decompression in MCUboot application upgrade scenarios."""

from __future__ import annotations

import logging
import os
from pathlib import Path

import pytest
from check_lzma_compression import check_lzma_compression
from mcuboot_image_utils import change_byte_in_tlv_area
from twister_harness import DeviceAdapter, MCUmgr, Shell
from twister_harness.helpers.utils import find_in_config
from upgrade_test_manager import UpgradeTestWithMCUmgr

logger = logging.getLogger(__name__)


def get_image_padding(dut: DeviceAdapter) -> int:
    """Get the image padding value for the device.

    Determines the ROM start offset from the device configuration.
    If partition manager is used, no padding is applied (returns 0).
    Otherwise, extracts the ROM start offset from Zephyr configuration.
    """
    assert dut.device_config.app_build_dir is not None, (
        "app_build_dir is required for compression tests"
    )

    pm_config = dut.device_config.build_dir / "pm.config"
    zephyr_config = dut.device_config.app_build_dir / "zephyr" / ".config"
    if not pm_config.exists():
        return int(find_in_config(zephyr_config, "CONFIG_ROM_START_OFFSET"), 16)
    else:
        return 0


def check_compressed_file_is_smaller(
    app_to_sign: str | Path, compressed_app: str | Path, padding: int
) -> None:
    """Check if the compressed application is smaller than the original."""
    app_to_sign_size = os.path.getsize(app_to_sign) - padding
    compressed_app_size = os.path.getsize(compressed_app)

    assert compressed_app_size < app_to_sign_size, (
        f"Compressed application {compressed_app} is not smaller than the original application "
        f"{app_to_sign}. Compressed application size: {compressed_app_size}, "
        f"Original application size: {app_to_sign_size}"
    )


def test_compressed_image_is_smaller(unlaunched_dut: DeviceAdapter):
    """Check that the compressed application is smaller than the original application."""
    assert unlaunched_dut.device_config.app_build_dir is not None, "app_build_dir is required"
    check_compressed_file_is_smaller(
        unlaunched_dut.device_config.app_build_dir / "zephyr" / "zephyr.bin",
        unlaunched_dut.device_config.app_build_dir / "zephyr" / "zephyr.signed.bin",
        get_image_padding(unlaunched_dut),
    )


def test_decompressed_data_is_identical_as_before_compression(
    unlaunched_dut: DeviceAdapter, tmpdir
):
    """Decompress the application (zephyr.signed.bin) and verify that it is identical to the
    original."""
    assert unlaunched_dut.device_config.app_build_dir is not None, "app_build_dir is required"
    check_lzma_compression(
        unlaunched_dut.device_config.app_build_dir / "zephyr" / "zephyr.signed.bin",
        unlaunched_dut.device_config.app_build_dir / "zephyr" / "zephyr.bin",
        workdir=tmpdir,
        padding=get_image_padding(unlaunched_dut),
    )


def test_nrf_compress_upgrade(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr):
    """Verify that the application can be updated.

    APP based on smp_svr, MCUboot is the primary bootloader.
    Upload the application image signed with a higher version.
        Confirm image. Reset DUT.
    Verify that the updated application is booted.
    """
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.increase_version()
    updated_app = tm.generate_image()

    check_compressed_file_is_smaller(
        tm.build_params.app_to_sign, updated_app, get_image_padding(dut)
    )

    tm.run_upgrade(updated_app, confirm=True)
    tm.verify_after_reset(
        lines=[
            "Erasing the primary slot",
            "Image 0 copying the secondary slot to the primary slot",
        ],
        no_lines=["Swap type: none"],
    )
    tm.check_with_shell_command(tm.get_current_sign_version())


@pytest.mark.nightly
@pytest.mark.parametrize("in_tlv_type", [0x72, 0x71], ids=["DECOMP_SIGNATURE", "DECOMP_SHA"])
def test_nrf_compress_flash_invalid(dut: DeviceAdapter, shell: Shell, mcumgr: MCUmgr, in_tlv_type):
    """Verify that the compressed application is not updated when TLV data is modified.

    APP based on smp_svr, MCUboot is the primary bootloader. LZMA compression is enabled.
    Generate an application update with a higher version.
        Modify the TLV data at offset for type 0x72 or 0x71.
        Upload the application update to slot 1 using mcumgr. Confirm image. Reset DUT.
    Verify that the application is not updated.
    """
    tm = UpgradeTestWithMCUmgr(dut, shell, mcumgr)
    tm.increase_version()
    updated_app = tm.generate_image()

    # Modify the TLV area for given type
    modified_app = change_byte_in_tlv_area(updated_app, in_tlv_type)

    tm.run_upgrade(modified_app, confirm=True)

    tm.verify_after_reset(
        lines=[
            "Image in the secondary slot is not valid",
        ],
        no_lines=["copying the secondary slot to the primary slot"],
    )
    tm.check_with_shell_command(tm.origin_mcuboot_version)
    logger.info(f"Invalid image with modified TLV 0x{in_tlv_type:x} not upgraded")
