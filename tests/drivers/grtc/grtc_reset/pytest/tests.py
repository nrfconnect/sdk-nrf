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

def get_cycle_and_uptime_from_logs(dut: DeviceAdapter):
    dut.clear_buffer()
    lines = dut.readlines_until(regex=r'.*k_uptime_get.*', timeout=5)
    for line in lines:
        match_cycle = re.search(r'k_cycle_get_32\s*=\s*(\d+)', line)
        if match_cycle:
            cycle=(int(match_cycle.group(1)))
        match_uptime = re.search(r'k_uptime_get\s*=\s*(\d+)', line)
        if match_uptime:
            uptime=(int(match_uptime.group(1)))
    return cycle, uptime

def test_grtc_after_reset_system(dut: DeviceAdapter):
    reset_dut(dut, reset_kind="RESET_PIN")
    cycle_start, uptime_start = get_cycle_and_uptime_from_logs(dut)
    assert uptime_start in range(8, 24)
    reset_dut(dut, reset_kind="RESET_SYSTEM")
    cycle_after_reset, uptime_after_reset = get_cycle_and_uptime_from_logs(dut)
    assert cycle_after_reset > cycle_start
    assert uptime_after_reset in range(8, 24)

def test_grtc_after_reset_pin(dut: DeviceAdapter):
    reset_dut(dut, reset_kind="RESET_PIN")
    cycle_start, uptime_start = get_cycle_and_uptime_from_logs(dut)
    assert uptime_start in range(8, 24)
    reset_dut(dut, reset_kind="RESET_PIN")
    cycle_after_reset, uptime_after_reset = get_cycle_and_uptime_from_logs(dut)
    assert abs(cycle_after_reset - cycle_start) < cycle_start / 2
    assert uptime_after_reset in range(8, 24)
