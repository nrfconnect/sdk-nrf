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


def test_west_flash(dut: DeviceAdapter):
    """
    Compile and flash test application on MCU.
    Start debug session by calling `west debug` command.
    Set brakepoint and check that breakpoint was hit.
    """
    BUILD_DIR = str(dut.device_config.build_dir)
    SEGGER_ID = dut.device_config.id
    LOG_TIMEOUT = 10.0

    # Wait a bit for the core to boot
    time.sleep(4)

    ### Check that code is executed
    dut.readlines_until(
        regex=r'.*west_flash: 5: Hello from',
        timeout=LOG_TIMEOUT,
    )

    gdb_port = find_free_port()

    ### Check that west -v flash -r jlink works
    cmd = (
        "west -v flash -r jlink"
        f" -d {BUILD_DIR} --skip-rebuild --"
        f" --dev-id {SEGGER_ID}"
        f" --gdb-port {gdb_port}"
    )
    expected = r"Flash download: Bank \d @ 0x\d+: Skipped. Contents already match"
    if "nrf54h20dk" in dut.device_config.platform:
        expected = r"O\.K\."
    run_communicate_check(
        cmd,
        None,
        # r"J-Link: Flash download: Total: [0-9.]+s",  # J-Link: Flash download: Total: 0.453s
        expected,
    )

    ### Check that code is executed
    dut.readlines_until(
        regex=r'.*<inf> west_flash: 5: Hello from',
        timeout=LOG_TIMEOUT,
    )
