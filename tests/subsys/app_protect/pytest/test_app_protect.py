#
# Copyright (c) 2026 Nordic Semiconductor ASA
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

DEVICEID = {
    'nrf52840dk/nrf52840': "0x10000060",
    'nrf5340dk/nrf5340/cpuapp': "0x00FF0204",
    'nrf54h20dk/nrf54h20/cpuapp': "0x0FFFE050",  # INFO.CONFIGID
    'nrf54h20dk@0.9.0/nrf54h20/cpuapp': "0x0FFFE050",  # INFO.CONFIGID
    'nrf54l15dk/nrf54l05/cpuapp': "0x00FFC304",
    'nrf54l15dk/nrf54l10/cpuapp': "0x00FFC304",
    'nrf54l15dk/nrf54l15/cpuapp': "0x00FFC304",
    'nrf54lm20dk/nrf54lm20a/cpuapp': "0x00FFC304",
    'nrf54ls05dk/nrf54ls05b/cpuapp': "0x00FFC304",
    'nrf54ls05dk@0.2.0/nrf54ls05b/cpuapp': "0x00FFC304",
    'nrf54lv10dk/nrf54lv10a/cpuapp': "0x00FFC304",
    'nrf54lv10dk@0.7.0/nrf54lv10a/cpuapp': "0x00FFC304",
    'nrf7120dk/nrf7120/cpuapp': "0x00FFC304",
}


# Kill parent process and all child processes (if started)
def _kill(proc):
    try:
        for child in psutil.Process(proc.pid).children(recursive=True):
            child.kill()
        proc.kill()
        logger.debug("Process was successfully killed.")
    except Exception as e:
        logger.exception(f"Could not kill process - {e}")


def print_output(stdouts, stderrs):
    if stdouts:
        for out in stdouts.split("\n"):
            logger.info(f"{out}")
    if stderrs:
        for err in stderrs.split("\n"):
            logger.error(f"{err}")


def run_communicate_check(cmd: str, input: str, regex: str):
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

    expected_str = re.search(regex, outs)
    assert expected_str is not None, f"Failed to match {regex} in {outs}"


def test_debug_interface(dut: DeviceAdapter):
    """
    Compile and flash test application on MCU.
    Repeat three times:
    - Do pinreset.
    - Wait for log on serial console.
    - Check that debug interface is opened (AppProtect is disabled).
    """
    PLATFORM = dut.device_config.platform
    SEGGER_ID = dut.device_config.id
    LOG_TIMEOUT = 10.0

    # Wait a bit for the core to boot
    time.sleep(4)

    for _ in range(3):
        dut.clear_buffer()

        ### Pinreset
        cmd = f"nrfutil device reset --reset-kind RESET_PIN --serial-number {SEGGER_ID}"
        expected = r".*"
        run_communicate_check(
            cmd,
            None,
            expected,
        )

        ### Check that code is executed
        dut.readlines_until(
            regex=r'.*west_flash: 3: Hello from',
            timeout=LOG_TIMEOUT,
        )

        ### Check that debug access port is open
        cmd = f"nrfutil device read --address {DEVICEID[PLATFORM]} --serial-number {SEGGER_ID}"
        expected = rf"{DEVICEID[PLATFORM]}: [A-F0-9]+"
        run_communicate_check(
            cmd,
            None,
            expected,
        )


def test_nrfjprog_recover(dut: DeviceAdapter):
    """
    Compile and flash test application on MCU.
    Wait for serial console log.
    Execute nrfutil device recover.
    Check that debug interface is opened (AppProtect is disabled).
    """
    PLATFORM = dut.device_config.platform
    SEGGER_ID = dut.device_config.id
    LOG_TIMEOUT = 10.0

    ### Check that code is executed
    dut.readlines_until(
        regex=r'.*west_flash: 3: Hello from',
        timeout=LOG_TIMEOUT,
    )

    ### Execute nrfutil device recover
    cmd = f"nrfutil device recover --serial-number {SEGGER_ID}"
    expected = r".*"
    run_communicate_check(
        cmd,
        None,
        expected,
    )

    ### Check that debug access port is open
    cmd = f"nrfutil device read --address {DEVICEID[PLATFORM]} --serial-number {SEGGER_ID}"
    expected = rf"{DEVICEID[PLATFORM]}: [A-F0-9]+"
    run_communicate_check(
        cmd,
        None,
        expected,
    )
