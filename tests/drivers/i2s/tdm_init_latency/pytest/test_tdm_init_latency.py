# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import logging
import subprocess
import time

from twister_harness import DeviceAdapter

logger = logging.getLogger("tdm_init_latency")
logger.setLevel(logging.DEBUG)


def pin_reset(dut: DeviceAdapter):
    """
    Apply DUT pin reset with nrfutil device
    """
    device_id: str = dut.device_config.id.lstrip("0")
    cmd: str = f"nrfutil device reset --reset-kind RESET_PIN --serial-number {device_id}"
    subprocess.call(cmd.split(" "))


def test_tdm_init_latency(dut: DeviceAdapter):
    """
    Roughly measure time from
    DUT Pin Reset to TDM ready message
    printed on console
    """

    lines = dut.readlines_until(regex="TDM ready", print_output=True, timeout=2)
    dut.clear_buffer()

    pin_reset(dut)
    start_time: float = time.perf_counter()

    lines = dut.readlines_until(regex="TDM ready", print_output=True, timeout=2)
    stop_time: float = time.perf_counter()

    logger.info(lines)

    logger.info(
        f"Time from Pin Reset to TDM ready message [ms]: {1000 *(stop_time - start_time):4.0f}"
    )
