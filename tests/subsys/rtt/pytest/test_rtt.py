#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import logging
import re
import subprocess
import time
from pathlib import Path

import psutil
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)

# Kill parent process and all child processes (if started)
def _kill(proc):
    try:
        for child in psutil.Process(proc.pid).children(recursive=True):
            child.kill()
        proc.kill()
    except Exception as e:
        logger.exception(f'Could not kill JLinkRTTLoggerExe - {e}')


def test_rtt_logging(dut: DeviceAdapter):
    """
    Compile and flash test application on MCU.
    Tested core(s) uses RTT backend for logging.
    JLinkRTTLoggerExe is used to collect logs.
    """
    BUILD_DIR = str(dut.device_config.build_dir)
    PLATFORM = dut.device_config.platform
    SEGGER_ID = dut.device_config.id
    COLLECT_TIMEOUT = 10.0
    EXPECTED = rf"log_rtt: \d+: Hello from {PLATFORM}"

    SWD_CONFIG = {
        'nrf52dk/nrf52832': {
            'device': 'nRF52832_xxAA',
        },
        'nrf52840dk/nrf52840': {
            'device': 'nRF52840_xxAA',
        },
        'nrf5340dk/nrf5340/cpuapp': {
            'device': 'nRF5340_xxAA_APP',
        },
        'nrf54l15dk/nrf54l05/cpuapp': {
            'device': 'nRF54L05_M33',
        },
        'nrf54l15dk/nrf54l10/cpuapp': {
            'device': 'nRF54L10_M33',
        },
        'nrf54l15dk/nrf54l15/cpuapp': {
            'device': 'nRF54L15_M33',
        },
        'nrf54lm20dk/nrf54lm20a/cpuapp': {
            'device': 'NRF54LM20A_M33',
        },
        'nrf54ls05dk@0.0.0/nrf54ls05b/cpuapp': {
            'device': 'NRF54LS05B_M33',
        },
        'nrf54lv10dk/nrf54lv10a/cpuapp': {
            'device': 'NRF54LV10A_M33',
        },
        'nrf54lv10dk@0.2.0/nrf54lv10a/cpuapp': {
            'device': 'NRF54LV10A_M33',
        },
        'nrf54lv10dk@0.7.0/nrf54lv10a/cpuapp': {
            'device': 'NRF54LV10A_M33',
        },
    }

    log_filename = f"{BUILD_DIR}/log_rtt.txt"
    try:
        Path(f"{log_filename}").unlink()
        logger.info("Old output file was deleted")
    except FileNotFoundError:
        pass

    # Wait a bit for the core to boot
    time.sleep(2)

    # use JLinkRTTLoggerExe to collect logs
    cmd = f"JLinkRTTLoggerExe -USB {SEGGER_ID}"
    cmd += f" -device {SWD_CONFIG[PLATFORM]['device']}"
    cmd += " -If SWD -Speed 1000 -RTTChannel 0"
    cmd += f" {log_filename}"

    try:
        logger.info(f"Executing:\n{cmd}")
        proc = subprocess.Popen(
            cmd.split(),
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            encoding='UTF-8',
        )
    except OSError as exc:
        logger.error(f"Unable to start JLinkRTTLoggerExe:\n{cmd=}\n{exc=}")

    try:
        proc.wait(COLLECT_TIMEOUT)
    except subprocess.TimeoutExpired:
        pass
    finally:
        _kill(proc)
        outs, errs = proc.communicate()
        logger.info(f"{outs=}\n{errs=}")

    # read logs
    with open(f"{log_filename}", errors="ignore") as log_file:
        log_file_content = log_file.read()

    # if nothing in log_file, stop test
    assert(
        len(log_file_content) > 0
    ), f"File {log_filename} is empty"

    # Check if log file contains expected string
    expected_str = re.search(EXPECTED, log_file_content)
    assert expected_str is not None, f"Failed to match {EXPECTED} in {log_filename}"
