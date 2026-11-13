# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import random
from pathlib import Path

import pytest
from fw_info_hex_utils import (
    _app_config_path,
    assert_fw_info_discovery_rejection,
    corrupt_fw_info_magic_patch,
    get_firmware_info_magic_bytes,
    get_fw_info_search_base,
    locate_fw_info_address,
)
from intelhex import IntelHex, bin2hex
from twister_harness import DeviceAdapter
from twister_harness_ext.adapters.nrfutil import flash_with_nrfutil, reset_board
from twister_harness_ext.utils.dts_helper import get_partition_address
from twister_harness_ext.utils.required_build import get_required_build


def create_dummy_file(path: str | Path, size: int):
    random_data = bytes([random.randint(1, 254) for _ in range(size)])
    with open(path, "wb") as file:
        file.write(random_data)


def test_program_invalid_binary_to_s0_slot(dut: DeviceAdapter, tmp_path: Path):
    """Verify if upload of an invalid image is rejected.

    - Build application with NSIB bootloader, without the MCUboot
    - Create 2 version of the firmware
    - Create one additional, invalid blob (random data, GIFs, etc.)
    - Program the application v1/s0
    - Transfer update to v2/s1
    - Apply the update and verify that the application boots from v2/s1
    - Transfer invalid blob into v1/s0
    - Reboot
    - The application v2/s1 is booted, update is rejected
    """
    lines = dut.readlines_until(regex="Hello World!", print_output=True, timeout=20)
    pytest.LineMatcher(lines).fnmatch_lines(["*Attempting to boot slot 0*", "*Firmware version 1*"])

    # build an image with higher firmware version
    build_dir = get_required_build(
        dut, extra_args="-- -DCONFIG_FW_INFO_FIRMWARE_VERSION=2", suffix="_v2"
    )

    flash_with_nrfutil(
        build_dir / f"signed_by_b0_{dut.device_config.app_build_dir.name}_s1_variant.hex",  # type: ignore
        dut.device_config.id,
        erase_mode="ERASE_RANGES_TOUCHED_BY_FIRMWARE",
    )
    reset_board(dut.device_config.id)

    lines = dut.readlines_until(regex="Hello World!", print_output=True, timeout=20)
    pytest.LineMatcher(lines).fnmatch_lines(["*Attempting to boot slot 1*", "*Firmware version 2*"])

    edt_data = dut.device_config.app_build_dir / "zephyr" / "edt.pickle"  # type: ignore
    s0_offset = get_partition_address(
        edt_data,
        "s0_partition",
        absolute=True,
    )

    invalid_bin_file = tmp_path / "blob.bin"
    invalid_hex_file = tmp_path / "blob.hex"
    create_dummy_file(invalid_bin_file, 30 * 1024)
    bin2hex(invalid_bin_file, invalid_hex_file, s0_offset)

    # program DUT with the invalid image
    flash_with_nrfutil(
        invalid_hex_file, dut.device_config.id, erase_mode="ERASE_RANGES_TOUCHED_BY_FIRMWARE"
    )
    reset_board(dut.device_config.id)
    lines = dut.readlines_until(regex="Hello World!", print_output=True, timeout=20)
    matcher = pytest.LineMatcher(lines)
    matcher.fnmatch_lines(
        [
            "*Attempting to boot slot 1*",
            "*Firmware version 2*",
        ]
    )
    matcher.no_fnmatch_line("*Attempting to boot slot 0*")
    matcher.no_fnmatch_line("*No fw_info struct found*")


@pytest.mark.nightly
def test_s0_corrupted_fw_info_magic_fallback_to_s1(dut: DeviceAdapter, tmp_path: Path):
    """NSIB skips s0 when fw_info magic is invalid; a valid signed image in s1 boots.

    Rejection must be due to firmware information discovery, not signature validation.
    """
    assert dut.device_config.app_build_dir is not None
    app_name = dut.device_config.app_build_dir.name
    edt_data = dut.device_config.app_build_dir / "zephyr" / "edt.pickle"

    # Initial state: the default build (v1) is flashed to s0 and booted.
    lines = dut.readlines_until(regex="Hello World!", print_output=True, timeout=20)
    pytest.LineMatcher(lines).fnmatch_lines(["*Attempting to boot slot 0*", "*Firmware version 1*"])

    # Build a second image with a higher fw_info version (v2).
    build_dir = get_required_build(
        dut, extra_args="-- -DCONFIG_FW_INFO_FIRMWARE_VERSION=2", suffix="_v2"
    )

    # Program v2 into s1; NSIB should prefer it over v1 in s0.
    flash_with_nrfutil(
        build_dir / f"signed_by_b0_{app_name}_s1_variant.hex",
        dut.device_config.id,
        erase_mode="ERASE_RANGES_TOUCHED_BY_FIRMWARE",
    )
    reset_board(dut.device_config.id)

    lines = dut.readlines_until(regex="Hello World!", print_output=True, timeout=20)
    pytest.LineMatcher(lines).fnmatch_lines(["*Attempting to boot slot 1*", "*Firmware version 2*"])

    # Program v2 into s0 as well; with equal versions NSIB boots s0.
    signed_v2_hex = build_dir / f"signed_by_b0_{app_name}.hex"
    flash_with_nrfutil(
        signed_v2_hex, dut.device_config.id, erase_mode="ERASE_RANGES_TOUCHED_BY_FIRMWARE"
    )
    reset_board(dut.device_config.id)

    lines = dut.readlines_until(regex="Hello World!", print_output=True, timeout=20)
    pytest.LineMatcher(lines).fnmatch_lines(["*Attempting to boot slot 0*", "*Firmware version 2*"])

    # Find the fw_info magic in the s0 image at the same address NSIB searches from.
    magic = get_firmware_info_magic_bytes(_app_config_path(dut))
    search_base = get_fw_info_search_base(dut, edt_data, "s0_partition")
    magic_addr = locate_fw_info_address(IntelHex(str(signed_v2_hex)), search_base, magic)

    # Overwrite only the magic bytes with 0xFF; the rest of the image stays intact.
    corrupted_s0_hex = IntelHex(str(signed_v2_hex))
    magic_patch = corrupt_fw_info_magic_patch(magic_addr)
    for address in magic_patch.addresses():
        corrupted_s0_hex[address] = magic_patch[address]

    corrupted_s0_hex_file = tmp_path / "s0_fw_info_magic_corrupt.hex"
    corrupted_s0_hex.write_hex_file(str(corrupted_s0_hex_file))

    # Reflash s0 with the corrupted image; s1 still holds the valid v2.
    flash_with_nrfutil(
        corrupted_s0_hex_file,
        dut.device_config.id,
        erase_mode="ERASE_RANGES_TOUCHED_BY_FIRMWARE",
    )
    reset_board(dut.device_config.id)

    # NSIB must fall back to s1 without attempting s0 or reporting signature failures.
    lines = dut.readlines_until(regex="Hello World!", print_output=True, timeout=20)
    matcher = pytest.LineMatcher(lines)
    matcher.fnmatch_lines(
        [
            "*Attempting to boot slot 1*",
            "*Firmware version 2*",
        ]
    )
    assert_fw_info_discovery_rejection(matcher)
