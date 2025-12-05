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


def west_tester(dut: DeviceAdapter, main_command: str, input_data: str, expected_regex: str):
    BUILD_DIR = str(dut.device_config.build_dir)
    SEGGER_ID = dut.device_config.id
    COLLECT_TIMEOUT = 15.0

    logger.debug(f"{dut.device_config=}")

    # Wait a bit for the core to boot
    time.sleep(4)

    gdb_port = find_free_port()
    cmd = (
        f"west {main_command}"
        f" -d {BUILD_DIR} --skip-rebuild --"
        f" --dev-id {SEGGER_ID}"
        f" --gdb-port {gdb_port}"
        f" --domain west_debug"
    )
    logger.info(f"Executing:\n{cmd}")
    proc = subprocess.Popen(
        cmd.split(),
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        encoding="UTF-8",
        text=True,
    )
    time.sleep(2)

    logger.info("Try to communicate with the process.")

    if "/ns" in dut.device_config.platform:
        # add tfm symbols
        input_data = f"add-symbol-file {BUILD_DIR}/west_debug/tfm/bin/tfm_s.elf\ny\n{input_data}"

    try:
        outs, errs = proc.communicate(
            input=input_data,
            timeout=COLLECT_TIMEOUT,
        )
    except subprocess.TimeoutExpired:
        logger.error("TimeoutExpired")
        _kill(proc)
    finally:
        outs, errs = proc.communicate()
        print_output(outs, errs)

    expected_str = re.search(expected_regex, outs)
    assert expected_str is not None, f"Failed to match {expected_regex} in {outs}"


def test_west_debug(dut: DeviceAdapter):
    """
    Compile and flash test application on MCU.
    Start debug session by calling `west debug` command.
    Set brakepoint and check that breakpoint was hit.
    """
    west_tester(dut, "debug", "b main.c:15\nc\ndisconnect\nq\n", r"15\s+counter\+\+;")


def test_west_attach(dut: DeviceAdapter):
    """
    Compile and flash test application on MCU.
    Cal `west atach` command.
    Check response to where.
    """
    west_tester(dut, "attach", "where\ndisconnect\nq\n", r"in k_busy_wait \(usec_to_wait=500000\)")
