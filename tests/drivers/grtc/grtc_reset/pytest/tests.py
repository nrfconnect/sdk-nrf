#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import logging
import re
import subprocess
import time

from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


platform_with_mcuboot_uptime_range_map = {
    "nrf54h20dk/nrf54h20/cpuapp": range(9, 11),
<<<<<<< Updated upstream
    "nrf54l15dk/nrf54l05/cpuapp": range(11, 21),
    "nrf54l15dk/nrf54l10/cpuapp": range(11, 21),
    "nrf54l15dk/nrf54l15/cpuapp": range(11, 21),
    "nrf54lm20dk/nrf54lm20a/cpuapp": range(11, 21),
    "nrf54lm20dk/nrf54lm20b/cpuapp": range(11, 21),
    "nrf54ls05dk/nrf54ls05a/cpuapp": range(9, 21),
    "nrf54ls05dk/nrf54ls05b/cpuapp": range(9, 21),
    "nrf54lv10dk/nrf54lv10a/cpuapp": range(11, 21),
=======
    "nrf54l15dk/nrf54l05/cpuapp": range(11, 13),
    "nrf54l15dk/nrf54l10/cpuapp": range(11, 13),
    "nrf54l15dk/nrf54l15/cpuapp": range(11, 13),
    "nrf54lm20dk/nrf54lm20a/cpuapp": range(11, 13),
    "nrf54lm20dk/nrf54lm20b/cpuapp": range(11, 13),
    "nrf54ls05dk/nrf54ls05a/cpuapp": range(9, 13),
    "nrf54ls05dk/nrf54ls05b/cpuapp": range(9, 13),
    "nrf54lv10dk/nrf54lv10a/cpuapp": range(11, 13),
    "nrf54lc10dk/nrf54lc10a/cpuapp": range(11, 13),
>>>>>>> Stashed changes
}

platform_without_mcuboot_uptime_range_map = {
    "nrf54h20dk/nrf54h20/cpuapp": range(9, 11),
<<<<<<< Updated upstream
    "nrf54l15dk/nrf54l05/cpuapp": range(9, 21),
    "nrf54l15dk/nrf54l10/cpuapp": range(9, 21),
    "nrf54l15dk/nrf54l15/cpuapp": range(9, 21),
    "nrf54lm20dk/nrf54lm20a/cpuapp": range(11, 21),
    "nrf54lm20dk/nrf54lm20b/cpuapp": range(11, 21),
    "nrf54ls05dk/nrf54ls05a/cpuapp": range(9, 21),
    "nrf54ls05dk/nrf54ls05b/cpuapp": range(9, 21),
    "nrf54lv10dk/nrf54lv10a/cpuapp": range(11, 21),
=======
    "nrf54l15dk/nrf54l05/cpuapp": range(9, 13),
    "nrf54l15dk/nrf54l10/cpuapp": range(9, 13),
    "nrf54l15dk/nrf54l15/cpuapp": range(9, 13),
    "nrf54lm20dk/nrf54lm20a/cpuapp": range(11, 13),
    "nrf54lm20dk/nrf54lm20b/cpuapp": range(11, 13),
    "nrf54ls05dk/nrf54ls05a/cpuapp": range(9, 13),
    "nrf54ls05dk/nrf54ls05b/cpuapp": range(9, 13),
    "nrf54lv10dk/nrf54lv10a/cpuapp": range(11, 13),
    "nrf54lc10dk/nrf54lc10a/cpuapp": range(11, 13),
>>>>>>> Stashed changes
}


def get_uptime_range(dut: DeviceAdapter, has_mcuboot: bool = False) -> range:
    platform = dut.device_config.platform
    at_index = platform.find("@")
    if at_index != -1:
        slash_index = platform.find("/", at_index)
        if slash_index != -1:
            platform = platform[:at_index] + platform[slash_index:]
    if has_mcuboot:
        range_obj = platform_with_mcuboot_uptime_range_map.get(platform)
    else:
        range_obj = platform_without_mcuboot_uptime_range_map.get(platform)
    if range_obj is None:
        raise ValueError(f"Unsupported platform {platform}, missing uptime range definition")
    return range_obj


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


def test_grtc_after_reset_system(dut: DeviceAdapter):
    time.sleep(3)
    cycle_start, uptime_start = reset_dut_and_get_cycle_and_uptime(dut, reset_kind="RESET_PIN")
    assert uptime_start in get_uptime_range(dut)
    cycle_after_reset, uptime_after_reset = reset_dut_and_get_cycle_and_uptime(
        dut, reset_kind="RESET_SYSTEM"
    )
    assert cycle_after_reset > cycle_start
    assert uptime_after_reset in get_uptime_range(dut)


def test_grtc_after_reset_pin(dut: DeviceAdapter):
    time.sleep(3)
    cycle_start, uptime_start = reset_dut_and_get_cycle_and_uptime(dut, reset_kind="RESET_PIN")
    assert uptime_start in get_uptime_range(dut)
    cycle_after_reset, uptime_after_reset = reset_dut_and_get_cycle_and_uptime(
        dut, reset_kind="RESET_PIN"
    )
    assert abs(cycle_after_reset - cycle_start) < cycle_start / 2
    assert uptime_after_reset in get_uptime_range(dut)


def test_mcuboot_grtc_uninit(dut: DeviceAdapter):
    time.sleep(3)
    _, uptime = reset_dut_and_get_cycle_and_uptime(dut, reset_kind="RESET_PIN")
    assert uptime in get_uptime_range(dut, has_mcuboot=False)


def test_mcuboot_grtc_no_uninit(dut: DeviceAdapter):
    time.sleep(3)
    _, uptime = reset_dut_and_get_cycle_and_uptime(dut, reset_kind="RESET_PIN")
    assert uptime in get_uptime_range(dut, has_mcuboot=True)
