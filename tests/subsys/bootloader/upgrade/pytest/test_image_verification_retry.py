# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Tests for the MCUboot image verification retry mechanism."""

import logging
import tempfile
from pathlib import Path

import intelhex
import pytest
from twister_harness import DeviceAdapter
from twister_harness.helpers.utils import find_in_config
from twister_harness_ext.utils.common import flash_with_nrfutil, go_cpu, halt_cpu, reset_board
from twister_harness_ext.utils.dts_helper import get_code_partition_address
from twister_harness_ext.utils.helpers import nrfutil_read, nrfutil_write

logger = logging.getLogger(__name__)
DAMAGE_VALUE = 0xDEADBEEF


def get_app_slot_address(dut: DeviceAdapter) -> str:
    """Calculate the application slot address.

    :param dut: The device adapter instance.
    :return: The application slot address as hex string.
    """
    if dut.device_config.app_build_dir is None:
        raise ValueError("app_build_dir is not set in device configuration")

    app_edt_pickle = dut.device_config.app_build_dir / "zephyr" / "edt.pickle"
    if not app_edt_pickle.exists():
        raise ValueError("edt.pickle not found in app build directory")
    app_addr = get_code_partition_address(app_edt_pickle, absolute=True)
    return app_addr


def get_app_image_address(dut: DeviceAdapter) -> str:
    """Calculate the application image address.

    :param dut: The device adapter instance.
    :return: The application image address as hex string.
    """
    if dut.device_config.app_build_dir is None:
        raise ValueError("app_build_dir is not set in device configuration")

    app_config_path = dut.device_config.app_build_dir / "zephyr" / ".config"
    flash_rom_start_offset = int(find_in_config(app_config_path, "CONFIG_ROM_START_OFFSET"), 0)
    app_edt_pickle = dut.device_config.app_build_dir / "zephyr" / "edt.pickle"
    if not app_edt_pickle.exists():
        raise ValueError("edt.pickle not found in app build directory")
    app_addr = get_code_partition_address(app_edt_pickle, absolute=True)
    return hex(int(app_addr, 0) + flash_rom_start_offset)


def program_app_image_page(dut: DeviceAdapter, modification_data: bytes | None = None) -> None:
    """Write data to the beginning of the application image on the device using nrfutil program.

    :param dut: The device adapter instance.
    :param modification_data: The data bytes to write, if None, program with original data.
    """
    page_size = 4096
    header_size = 512

    if dut.device_config.app_build_dir is None:
        raise ValueError("app_build_dir is not set in device configuration")

    signed_bin_path = dut.device_config.app_build_dir / "zephyr" / "zephyr.signed.bin"

    app_address = get_app_slot_address(dut)
    slot_offset = int(app_address, 0)

    with open(signed_bin_path, "rb") as original_file:
        page_data = bytearray(original_file.read(page_size))
        if modification_data is not None:
            page_data[header_size : header_size + len(modification_data)] = modification_data

        with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as temp_bin_file:
            temp_bin_file.write(page_data)
            temp_bin_path = temp_bin_file.name

        temp_hex_path = temp_bin_path.replace(".bin", ".hex")
        ih = intelhex.IntelHex()
        ih.loadbin(temp_bin_path, offset=slot_offset)
        ih.write_hex_file(temp_hex_path)

        logger.info(f"Flashing modified hex file: {temp_hex_path}")
        flash_with_nrfutil(
            temp_hex_path, dut.device_config.id, erase_mode="ERASE_RANGES_TOUCHED_BY_FIRMWARE"
        )

        Path(temp_bin_path).unlink(missing_ok=True)
        Path(temp_hex_path).unlink(missing_ok=True)


def damage_image(dut: DeviceAdapter) -> str:
    """Corrupt the application image and return the original data.

    :param dut: The device adapter instance.
    :return: The original data that was overwritten.
    """
    app_address = get_app_image_address(dut)

    logger.info(f"Reading original data from address {app_address}")
    halt_cpu(dut.device_config.id)
    try:
        hex_data = nrfutil_read(f"{app_address}", 4, dut.device_config.id)
        logger.info(f"Original data: {hex_data}")

        logger.info(f"Damaging application at address {app_address}")
        if "nrf54" in dut.device_config.platform:
            nrfutil_write(f"{app_address}", f"0x{DAMAGE_VALUE:x}", dut.device_config.id)
        else:
            # for nRF53 and nRF52 platforms where NVMC works
            # need to erase the page to write memory
            damage_bytes = DAMAGE_VALUE.to_bytes(4, byteorder="little")
            program_app_image_page(dut, damage_bytes)
        return hex_data
    finally:
        go_cpu(dut.device_config.id)


def repair_image(dut: DeviceAdapter, original_data: str):
    """Restore the original data to the application image.

    :param dut: The device adapter instance.
    :param original_data: The original data (as a hex string) to write back.
    """
    app_address = get_app_image_address(dut)
    logger.info(f"Restoring original data {original_data} to address {app_address}")
    halt_cpu(dut.device_config.id)
    try:
        if "nrf54" in dut.device_config.platform:
            nrfutil_write(f"{app_address}", original_data, dut.device_config.id)
        else:
            # for nRF53 and nRF52 platforms where NVMC works need to restore
            # entire first app image page to recover
            program_app_image_page(dut)
    finally:
        go_cpu(dut.device_config.id)


def test_image_validation_fails_with_permanent_damage(dut: DeviceAdapter):
    """Verify that the application does not boots after the number of retries
    when the image is permanently damaged.
    """
    dut.readlines_until(regex="Booting nRF Connect SDK", timeout=5.0)
    original_data = damage_image(dut)
    reset_board(dut.device_config.id)

    try:
        lines = dut.readlines_until(regex="Image in the primary slot is not valid!", timeout=22.0)
        pytest.LineMatcher(lines).fnmatch_lines(["*Image validation attempt 3/3 failure: -1*"])
    finally:
        repair_image(dut, original_data)
        reset_board(dut.device_config.id)
        dut.readlines_until(regex="Booting nRF Connect SDK", timeout=12.0)


def test_image_validation_succeeds_on_retry_after_transient_fault(dut: DeviceAdapter):
    """Verify that the application boots successfully after a transient fault
    is repaired in the retry window.
    """
    dut.readlines_until(regex="Booting nRF Connect SDK", timeout=5.0)
    original_data = damage_image(dut)
    reset_board(dut.device_config.id)
    dut.readlines_until(regex="Image validation attempt 1/3 failure: -1", timeout=12.0)
    repair_image(dut, original_data)
    lines = dut.readlines_until(regex="Booting nRF Connect SDK", timeout=12.0)
    pytest.LineMatcher(lines).no_fnmatch_line("*Image in the primary slot is not valid!*")


def test_normal_boot_unaffected_by_retry_mechanism(dut: DeviceAdapter):
    """Verify that a normal boot is not affected by the retry mechanism."""
    dut.readlines_until(regex="Booting nRF Connect SDK", timeout=5.0)
    reset_board(dut.device_config.id)
    lines = dut.readlines_until(regex="Booting nRF Connect SDK", timeout=5.0)
    pytest.LineMatcher(lines).no_fnmatch_line("*Image validation attempt * failure*")
