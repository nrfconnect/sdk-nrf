# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""E2E NSIB tests for FW_INFO discovery."""

from __future__ import annotations

from pathlib import Path

import pytest
from fw_info_hex_utils import (
    FW_INFO_ALLOWED_OFFSETS,
    edt_pickle_for_build,
    get_firmware_info_magic_bytes,
    get_fw_info_search_base,
    locate_fw_info_address,
)
from intelhex import IntelHex
from twister_harness import DeviceAdapter
from twister_harness_ext.adapters.nrfutil import flash_with_nrfutil, reset_board
from twister_harness_ext.utils.required_build import (
    RequiredBuildException,
    get_required_build,
)


def _variant_build_dir(dut: DeviceAdapter, suffix: str) -> Path:
    return Path(str(dut.device_config.build_dir) + suffix)


@pytest.mark.parametrize(
    "fw_info_offset",
    [hex(offset) for offset in FW_INFO_ALLOWED_OFFSETS],
    ids=[hex(offset) for offset in FW_INFO_ALLOWED_OFFSETS],
)
def test_boot_with_fw_info_at_each_allowed_offset(
    unlaunched_dut: DeviceAdapter, fw_info_offset: str
):
    """Signed application boots when fw_info is linked at each permitted offset.

    Not every documented discovery offset can be linked: fw_info must not overlap
    the vector table or precede the ROM_START_OFFSET padding (the linker rejects
    such CONFIG_FW_INFO_OFFSET values). Those cases are skipped.
    """
    assert unlaunched_dut.device_config.app_build_dir is not None
    app_name = unlaunched_dut.device_config.app_build_dir.name
    suffix = f"_off_{fw_info_offset.lower().replace('0x', '')}"

    try:
        build_dir = get_required_build(
            unlaunched_dut,
            extra_args=f"-- -DCONFIG_FW_INFO_OFFSET={fw_info_offset} "
            "-DCONFIG_FW_INFO_FIRMWARE_VERSION=2",
            suffix=suffix,
        )
    except RequiredBuildException:
        # Some Kconfig-legal offsets fail at link time with
        # `cannot move location counter backwards` (first reported for the
        # `b0_hello_world` application image; the s1 variant shares the same Kconfig).
        # fw_info is placed at CONFIG_FW_INFO_OFFSET + CONFIG_ROM_START_OFFSET (fw_info.ld)
        # inside the rom_start section:
        # - 0x0 never links: fw_info sorts before rom_start_offset.ld, which then
        #   tries to move `.` back to CONFIG_ROM_START_OFFSET.
        # - Offsets smaller than the vector table overlap it, e.g. 0x200 and 0x400
        #   on nRF54L15, whose vector table is 0x47C bytes.
        # Discovery still supports those offsets in general; this sample simply
        # cannot be linked at every one.
        variant_dir = _variant_build_dir(unlaunched_dut, suffix)
        build_log = variant_dir / "_build_log.txt"
        searched_msg = "cannot move location counter backwards"
        if build_log.exists() and searched_msg in build_log.read_text():
            (variant_dir / "build.error").unlink(missing_ok=True)
            pytest.skip(
                f"CONFIG_FW_INFO_OFFSET={fw_info_offset} is not linkable for b0_hello_world "
                f"on {unlaunched_dut.device_config.platform}"
            )
        else:
            raise

    hex_path = build_dir / f"signed_by_b0_{app_name}.hex"
    assert hex_path.is_file(), f"Missing signed image {hex_path}"
    edt_pickle = edt_pickle_for_build(unlaunched_dut, build_dir)
    assert edt_pickle.is_file()

    magic = get_firmware_info_magic_bytes(build_dir / app_name / "zephyr" / ".config")
    search_base = get_fw_info_search_base(unlaunched_dut, edt_pickle, "s0_partition")
    expected_addr = search_base + int(fw_info_offset, 16)
    actual_addr = locate_fw_info_address(IntelHex(str(hex_path)), search_base, magic)
    assert actual_addr == expected_addr, (
        f"fw_info found at 0x{actual_addr:x}, expected 0x{expected_addr:x}"
    )

    # launch() flashes the default build (b0, provisioning, default app) via west flash,
    # so the variant image must be programmed afterwards and the board reset.
    unlaunched_dut.launch()
    unlaunched_dut.readlines_until(regex="Hello World!", timeout=60)

    flash_with_nrfutil(
        hex_path,
        unlaunched_dut.device_config.id,
        erase_mode="ERASE_RANGES_TOUCHED_BY_FIRMWARE",
    )
    reset_board(unlaunched_dut.device_config.id)

    lines = unlaunched_dut.readlines_until(regex="Hello World!", timeout=60)
    pytest.LineMatcher(lines).fnmatch_lines(
        [
            "*Attempting to boot slot 0*",
            "*Firmware version 2*",
        ]
    )
