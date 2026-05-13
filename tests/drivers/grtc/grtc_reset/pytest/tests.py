#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import json
import logging
import os
import re
import subprocess
import time

from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


def get_uptime_range(dut: DeviceAdapter, limits_path: str, has_mcuboot: bool = False) -> range:
    with open(limits_path, encoding='utf-8') as file:
        limits_data = json.load(file)
    platform = dut.device_config.platform
    at_index = platform.find("@")
    if at_index != -1:
        slash_index = platform.find("/", at_index)
        if slash_index != -1:
            platform = platform[:at_index] + platform[slash_index:]
    if has_mcuboot:
        range_list = limits_data["platform_with_mcuboot_uptime_range_map"].get(platform)
    else:
        range_list = limits_data["platform_without_mcuboot_uptime_range_map"].get(platform)
    if range_list is None:
        raise ValueError(f"Unsupported platform {platform}, missing uptime range definition")
    return range(range_list[0], range_list[1])


def reset_dut(dut: DeviceAdapter, reset_kind: str = "RESET_PIN"):
    cmd = [
        "nrfutil",
        "device",
        "reset",
        "--reset-kind",
        reset_kind,
        "--serial-number",
        dut.device_config.id.lstrip("0"),
    ]
    logger.info(" ".join(cmd))
    output = subprocess.check_output(cmd, stderr=subprocess.STDOUT, timeout=5).decode("utf-8")
    logger.info(output)


def get_cycle_and_uptime_from_logs(dut: DeviceAdapter) -> tuple[int, int]:
    lines = dut.readlines_until(regex=r".*k_uptime_get.*", timeout=5)
    for line in lines:
        match_cycle = re.search(r"k_cycle_get_32\s*=\s*(\d+)", line)
        if match_cycle:
            cycle = int(match_cycle.group(1))
        match_uptime = re.search(r"k_uptime_get\s*=\s*(\d+)", line)
        if match_uptime:
            uptime = int(match_uptime.group(1))
    return cycle, uptime


def reset_dut_and_get_cycle_and_uptime(dut: DeviceAdapter, reset_kind: str) -> tuple[int, int]:
    dut.clear_buffer()
    reset_dut(dut, reset_kind)
    return get_cycle_and_uptime_from_logs(dut)


def test_grtc_after_reset_system(dut: DeviceAdapter, limits_path: str):
    time.sleep(3)
    cycle_start, uptime_start = reset_dut_and_get_cycle_and_uptime(dut, reset_kind="RESET_PIN")
    assert uptime_start in get_uptime_range(dut, limits_path)
    cycle_after_reset, uptime_after_reset = reset_dut_and_get_cycle_and_uptime(
        dut, reset_kind="RESET_SYSTEM"
    )
    assert cycle_after_reset > cycle_start
    assert uptime_after_reset in get_uptime_range(dut, limits_path)


def test_grtc_after_reset_pin(dut: DeviceAdapter, limits_path: str):
    time.sleep(3)
    cycle_start, uptime_start = reset_dut_and_get_cycle_and_uptime(dut, reset_kind="RESET_PIN")
    assert uptime_start in get_uptime_range(dut, limits_path)
    cycle_after_reset, uptime_after_reset = reset_dut_and_get_cycle_and_uptime(
        dut, reset_kind="RESET_PIN"
    )
    assert abs(cycle_after_reset - cycle_start) < cycle_start / 2
    assert uptime_after_reset in get_uptime_range(dut, limits_path)


def test_mcuboot_grtc_uninit(dut: DeviceAdapter, limits_path: str):
    time.sleep(3)
    _, uptime = reset_dut_and_get_cycle_and_uptime(dut, reset_kind="RESET_PIN")
    assert uptime in get_uptime_range(dut, limits_path, has_mcuboot=True)


def test_mcuboot_grtc_no_uninit(dut: DeviceAdapter, limits_path: str):
    time.sleep(3)
    _, uptime = reset_dut_and_get_cycle_and_uptime(dut, reset_kind="RESET_PIN")
    assert uptime in get_uptime_range(dut, limits_path, has_mcuboot=True)
