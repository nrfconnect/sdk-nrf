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

ROOT_DIR = Path(__file__).absolute().parents[0]
BC_SWO_ENABLE = Path(ROOT_DIR, 'nrf54l15_dk_pca10156_0.10.0_swo_enable.json')
BC_SWO_DEFAULT = Path(ROOT_DIR, 'nrf54l15_dk_pca10156_0.10.0_defaults.json')


# Kill parent process and all child processes (if started)
def _kill(proc):
    try:
        for child in psutil.Process(proc.pid).children(recursive=True):
            child.kill()
        proc.kill()
    except Exception as e:
        logger.exception(f"Could not kill JLinkSWOViewerCLExe - {e}")


def _reconfigure_board_controller(segger_id: int, cfg_file: Path):
    cmd_bc = [
        'nrfutil',
        'device',
        'x-execute-batch',
        '--traits',
        'boardController',
        '--serial-number',
        segger_id,
        '--batch-path',
        cfg_file.as_posix(),
    ]

    logger.info(f"Executing:\n{cmd_bc}")
    try:
        subprocess.run(cmd_bc, shell=False, encoding='UTF-8')
    except OSError as ose:
        logger.exception(f"Unable to configure Board Controller\n{cmd_bc}\n{ose}")


def test_swo_logging(dut: DeviceAdapter):
    """
    Compile and flash test application on MCU.
    Tested core(s) uses SWO backend for logging.
    JLinkSWOViewerCLExe is used to collect logs.
    """
    BUILD_DIR = str(dut.device_config.build_dir)
    PLATFORM = dut.device_config.platform
    SEGGER_ID = dut.device_config.id
    COLLECT_TIMEOUT = 10.0
    EXPECTED = rf"log_swo: \d+: Hello from {PLATFORM}"

    TRACEPORT_JLINK_SCRIPT = Path(__file__).parent.resolve() / "traceport_swo.script"

    logger.debug(f"{dut.device_config=}")

    SWO_CONFIG = {
        "nrf52dk/nrf52832": {
            "device": "nRF52832_xxAA",
            "cpufreq": 64000000,
            "swofreq": 1000000,
            "args": "",
        },
        "nrf52840dk/nrf52840": {
            "device": "nRF52840_xxAA",
            "cpufreq": 64000000,
            "swofreq": 1000000,
            "args": "",
        },
        "nrf5340dk/nrf5340/cpuapp": {
            "device": "nRF5340_xxAA_APP",
            "cpufreq": 64000000,
            "swofreq": 1000000,
            "args": "",
        },
        "nrf54l15dk/nrf54l05/cpuapp": {
            "device": "nRF54L05_M33",
            "cpufreq": 128000000,
            "swofreq": 1000000,
            "args": f"-jlinkscriptfile {TRACEPORT_JLINK_SCRIPT}",
        },
        "nrf54l15dk/nrf54l10/cpuapp": {
            "device": "nRF54L10_M33",
            "cpufreq": 128000000,
            "swofreq": 1000000,
            "args": f"-jlinkscriptfile {TRACEPORT_JLINK_SCRIPT}",
        },
        "nrf54l15dk/nrf54l15/cpuapp": {
            "device": "nRF54L15_M33",
            "cpufreq": 128000000,
            "swofreq": 1000000,
            "args": f"-jlinkscriptfile {TRACEPORT_JLINK_SCRIPT}",
        },
        "nrf54lm20dk/nrf54lm20a/cpuapp": {
            "device": "NRF54LM20A_M33",
            "cpufreq": 128000000,
            "swofreq": 1000000,
            "args": f"-jlinkscriptfile {TRACEPORT_JLINK_SCRIPT}",
        },
        "nrf54lm20dk/nrf54lm20b/cpuapp": {
            "device": "NRF54LM20A_M33",
            "cpufreq": 128000000,
            "swofreq": 1000000,
            "args": f"-jlinkscriptfile {TRACEPORT_JLINK_SCRIPT}",
        },
        "nrf54lv10dk/nrf54lv10a/cpuapp": {
            "device": "NRF54LV10A_M33",
            "cpufreq": 128000000,
            "swofreq": 1000000,
            "args": f"-jlinkscriptfile {TRACEPORT_JLINK_SCRIPT}",
        },
        "nrf54lc10dk@0.8.0/nrf54lc10a/cpuapp": {
            "device": "NRF54LV10A_M33",
            "cpufreq": 128000000,
            "swofreq": 1000000,
            "args": f"-jlinkscriptfile {TRACEPORT_JLINK_SCRIPT}",
        },
        "nrf54lv10dk@0.7.0/nrf54lv10a/cpuapp/ns": {
            "device": "NRF54LV10A_M33",
            "cpufreq": 128000000,
            "swofreq": 1000000,
            "args": f"-jlinkscriptfile {TRACEPORT_JLINK_SCRIPT}",
        },
        "nrf54lv10dk@0.7.0/nrf54lv10a/cpuapp": {
            "device": "NRF54LV10A_M33",
            "cpufreq": 128000000,
            "swofreq": 1000000,
            "args": f"-jlinkscriptfile {TRACEPORT_JLINK_SCRIPT}",
        },
        "nrf7120dk/nrf7120/cpuapp": {
            "device": "CORTEX_M33",
            "cpufreq": 256000000,
            "swofreq": 1000000,
            "args": f"-jlinkscriptfile {TRACEPORT_JLINK_SCRIPT}",
        },
    }

    log_filename = f"{BUILD_DIR}/log_swo.txt"
    try:
        Path(f"{log_filename}").unlink()
        logger.info("Old output file was deleted")
    except FileNotFoundError:
        pass

    # Wait a bit for the core to boot
    time.sleep(2)

    # Enable SWO pin on Board Controller
    # This is necessary on NRF54L15 PCA10156 in revision 0.10.0 and above
    if "nrf54l15dk" in PLATFORM and "swo_handling" in dut.device_config.fixtures:
        _reconfigure_board_controller(SEGGER_ID, BC_SWO_ENABLE)

    # use JLinkSWOViewerCLExe to collect logs
    cmd = f"JLinkSWOViewerCLExe -USB {SEGGER_ID}"
    cmd += f" -device {SWO_CONFIG[PLATFORM]['device']}"
    cmd += f" -cpufreq {SWO_CONFIG[PLATFORM]['cpufreq']}"
    cmd += f" -swofreq {SWO_CONFIG[PLATFORM]['swofreq']}"
    cmd += f" -itmmask 0xFFFF -outputfile {log_filename}"
    cmd += f" {SWO_CONFIG[PLATFORM]['args']}"
    try:
        logger.info(f"Executing:\n{cmd}")
        proc = subprocess.Popen(
            cmd.split(),
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            encoding='UTF-8',
            errors='replace',
        )
    except OSError as exc:
        logger.error(f"Unable to start JLinkSWOViewerCLExe:\n{cmd=}\n{exc=}")

    try:
        proc.wait(COLLECT_TIMEOUT)
    except subprocess.TimeoutExpired:
        pass
    finally:
        _kill(proc)
        outs, errs = proc.communicate()
        logger.info(f"{outs=}\n{errs=}")

    # Restore default Board Controller configuration on NRF54L15 PCA10156
    if "nrf54l15dk" in PLATFORM and "swo_handling" in dut.device_config.fixtures:
        _reconfigure_board_controller(SEGGER_ID, BC_SWO_DEFAULT)

    # read logs
    with open(f"{log_filename}", errors="ignore") as log_file:
        log_file_content = log_file.read()

    # if nothing in log_file, stop test
    assert len(log_file_content) > 0, f"File {log_filename} is empty"

    # Check if log file contains expected string
    expected_str = re.search(EXPECTED, log_file_content)
    assert expected_str is not None, f"Failed to match {EXPECTED} in {log_filename}"
