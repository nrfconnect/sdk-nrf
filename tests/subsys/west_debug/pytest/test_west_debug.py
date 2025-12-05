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


def test_west_debug(dut: DeviceAdapter):
    """
    Compile and flash test application on MCU.
    Start debug session by calling `west debug` command.
    Set brakepoint and check that breakpoint was hit.
    """
    BUILD_DIR = str(dut.device_config.build_dir)
    SEGGER_ID = dut.device_config.id
    COLLECT_TIMEOUT = 15.0
    EXPECTED = r"15\s+counter\+\+;"

    logger.debug(f"{dut.device_config=}")

    # Wait a bit for the core to boot
    time.sleep(4)

    gdb_port = int(SEGGER_ID) % 1000 + 2331

    # Check if west debug works
    cmd = f"west debug -d {BUILD_DIR} -- --dev-id {SEGGER_ID} --gdb-port {gdb_port}"
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
        # set breakpoint in main.c line 18 which is `counter++;`
        # continue
        # [breakpoint shall hit]
        # quit
        outs, errs = proc.communicate(
            input="b main.c:15\nc\ndisconnect\nq\n",
            timeout=COLLECT_TIMEOUT,
        )
    except subprocess.TimeoutExpired:
        _kill(proc)
    finally:
        outs, errs = proc.communicate()
        logger.info(f"{outs=}\n{errs=}")

    expected_str = re.search(EXPECTED, outs)
    assert expected_str is not None, f"Failed to match {EXPECTED} in {outs}"


def test_west_attach(dut: DeviceAdapter):
    """
    Compile and flash test application on MCU.
    Cal `west atach` command.
    Check response to where.
    """
    BUILD_DIR = str(dut.device_config.build_dir)
    SEGGER_ID = dut.device_config.id
    COLLECT_TIMEOUT = 15.0
    EXPECTED = r"in k_busy_wait \(usec_to_wait=500000\)"

    # Wait a bit for the core to boot
    time.sleep(4)

    gdb_port = int(SEGGER_ID) % 1000 + 2331
    cmd = f"west attach -d {BUILD_DIR} -- --dev-id {SEGGER_ID} --gdb-port {gdb_port}"

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
        # where
        # quit
        outs, errs = proc.communicate(input="where\ndisconnect\nq\n", timeout=COLLECT_TIMEOUT)
    except subprocess.TimeoutExpired:
        _kill(proc)
    finally:
        outs, errs = proc.communicate()
        logger.info(f"{outs=}\n{errs=}")

    expected_str = re.search(EXPECTED, outs)
    assert expected_str is not None, f"Failed to match {EXPECTED} in {outs}"
