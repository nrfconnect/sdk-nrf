#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import logging
import re
import socket
import subprocess
import time
from contextlib import closing

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


# get free port
# bind to port 0 and get free port from system
def find_free_port():
    with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as s:
        s.bind(("", 0))
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        return s.getsockname()[1]


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
    BUILD_DIR = str(dut.device_config.build_dir)
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

    gdb_port = find_free_port()

    ### Check that west attach doesn't work
    run_communicate_check(
        f"west attach -d {BUILD_DIR} -- --dev-id {SEGGER_ID} --gdb-port {gdb_port}",
        "where\ndisconnect\nq\n",
        r"Could not connect to target",
    )

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
