#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import logging
import re
import subprocess
import time

import psutil
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


# Kill parent process and all child processes (if started)
def _kill(proc):
    try:
        for child in psutil.Process(proc.pid).children(recursive=True):
            child.kill()
        proc.kill()
        logger.debug("Process was killed.")
    except Exception as e:
        logger.exception(f"Could not kill process - {e}")


def print_output(outs, errs):
    if outs:
        for out in outs.split("\n"):
            logger.info(f"{out}")
    if errs:
        for err in errs.split("\n"):
            logger.error(f"{err}")


def run_communicate_check(cmd: str, input: str, expected: str):
    logger.info(f"Executing:\n{cmd}")
    proc = subprocess.Popen(
        cmd.split(),
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding='UTF-8',
        text=True,
    )

    time.sleep(2)
    logger.info("Try to communicate with the process.")

    try:
        outs, errs = proc.communicate(input=input, timeout=5.0)
    except subprocess.TimeoutExpired:
        logger.error("TimeoutExpired")
        _kill(proc)
    finally:
        outs, errs = proc.communicate()
        print_output(outs, errs)

    expected_str = re.search(expected, outs)
    assert expected_str is not None, f"Failed to match {expected} in {outs}"


def test_swd(dut: DeviceAdapter):
    """
    Compile and flash test application on MCU.
    Check that SWD interface is disabled.
    Check that nrfutil device can temporarily enable SWD interface.
    """
    PLATFORM = dut.device_config.platform
    SEGGER_ID = dut.device_config.id
    ZTEST_LOG_TIMEOUT = 10.0

    # Wait a bit for the core to boot
    time.sleep(4)

    dut.readlines_until(
        regex=r'.*SUITE PASS - 100.00% \[gpio_swd\]: pass = \d+, fail = 0, skip = 0, total = \d+',
        timeout=ZTEST_LOG_TIMEOUT,
    )

    # This feature (switch SWD pins to GPIO mode) is not present on nrf54l15
    if PLATFORM == "nrf54l15dk/nrf54l15/cpuapp":
        return

    ### Check that nrfutil device can't connect
    run_communicate_check(
        f"nrfutil device core-info --serial-number {SEGGER_ID}",
        None,
        r"Setting the debug port SELECT register failed",
    )

    ### Temporarily enable SWD interface
    run_communicate_check(
        f"nrfutil device x-gpio-swd-muxing-disable --serial-number {SEGGER_ID}",
        None,
        r"^$",
    )

    ### Check that nrfutil device can connect
    run_communicate_check(
        f"nrfutil device core-info --serial-number {SEGGER_ID}",
        None,
        r"codeSize: \d+",
    )
