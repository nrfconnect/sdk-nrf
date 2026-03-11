# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import random
from pathlib import Path

import pytest
from intelhex import bin2hex
from twister_harness import DeviceAdapter
from twister_harness_ext.utils.common import flash_with_nrfutil
from twister_harness_ext.utils.helpers import reset_board
from twister_harness_ext.utils.required_build import get_required_build


def create_dummy_file(path: str | Path, size: int):
    random_data = bytes([random.randint(1, 254) for _ in range(size)])
    with open(path, "wb") as file:
        file.write(random_data)


def test_program_invalid_binary_to_s0_slot(dut: DeviceAdapter, config_reader, tmp_path: Path):
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
        build_dir / "signed_by_b0_s1_image.hex",
        dut.device_config.id,
        erase_mode="ERASE_RANGES_TOUCHED_BY_FIRMWARE",
    )
    reset_board(dut.device_config.id)

    lines = dut.readlines_until(regex="Hello World!", print_output=True, timeout=20)
    pytest.LineMatcher(lines).fnmatch_lines(["*Attempting to boot slot 1*", "*Firmware version 2*"])

    # create an invalid binary file
    s0_offset = config_reader(dut.device_config.build_dir / "pm.config").read_int("PM_S0_OFFSET")
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
