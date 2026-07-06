# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""System tests for tampered application security scenarios."""

import pytest
from intelhex import IntelHex
from twister_harness import DeviceAdapter
from twister_harness_ext.utils.dts_helper import get_partition_address


def test_tamper_s0_boot_s1(unlaunched_dut: DeviceAdapter):
    """Verify that the application in slot 1 boots when slot 0 is tampered.

    APP based on hello_world, the application in slot 0 is tampered before flashing the DUT.
    Image in slot 1 boots correctly. Image in slot 0 is permanently invalidated.
    """
    app_name = unlaunched_dut.device_config.app_build_dir.name  # type: ignore
    app_hex_path = unlaunched_dut.device_config.build_dir / f"signed_by_b0_{app_name}.hex"
    app_hex = IntelHex(str(app_hex_path))
    s0_address: int = get_partition_address(
        unlaunched_dut.device_config.app_build_dir / "zephyr/edt.pickle",  # type: ignore
        "s0_partition",
        absolute=True,
    )
    # Tamper the application image by modifying a byte at the beginning of slot 0
    app_hex.puts(s0_address, b"\xff")
    app_hex.write_hex_file(app_hex_path)

    unlaunched_dut.launch()
    lines = unlaunched_dut.readlines_until(regex="Hello World!")
    pytest.LineMatcher(lines).fnmatch_lines(
        [
            "*Attempting to boot slot 0*",
            "*Failed to validate, permanently invalidating!*",
            "*Attempting to boot slot 1*",
            "*I: Firmware signature verified*",
        ]
    )
