#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import os
import logging
import subprocess
import re
import shlex

from twister_harness import DeviceAdapter
from twister_harness.helpers.utils import find_in_config

logger = logging.getLogger(__name__)

def run_command(command: list[str], timeout: int = 30):
    try:
        ret: subprocess.CompletedProcess = subprocess.run(command,
                                                          text=True,
                                                          stdout=subprocess.PIPE,
                                                          stderr=subprocess.STDOUT,
                                                          timeout=timeout)
    except subprocess.TimeoutExpired:
        logger.error(f"Timeout expired for command: {shlex.join(command)}")
        raise

    if ret.returncode:
        logger.error(f"Command failed: {shlex.join(command)}")
        logger.error(ret.stdout)
        raise subprocess.CalledProcessError(ret.returncode, command)


def _mcuboot_key_path(dut: DeviceAdapter):
    mcuboot_conf_path = os.path.join(str(dut.device_config.build_dir), "mcuboot", "zephyr",
                                     ".config")
    return find_in_config(mcuboot_conf_path, "CONFIG_BOOT_SIGNATURE_KEY_FILE")


def mcuboot_provision(dut: DeviceAdapter):
    soc = dut.device_config.platform.split("/")[1]
    key_path = _mcuboot_key_path(dut)
    command = ["west", "ncs-provision", "upload", "-s", soc, "-k", key_path]
    if dut.device_config.id:
        command.extend(["--dev-id", dut.device_config.id])

    logger.info("KMU provisioning")
    run_command(command)


def board_flash(dut: DeviceAdapter):
    build_dir = dut.device_config.build_dir
    dev_id = dut.device_config.id

    command = ["west", "flash", "--skip-rebuild", "-d", build_dir]
    if dev_id:
        command.extend(["--dev-id", dev_id])

    logger.info("Programming the board")
    run_command(command)


def logs_verify(dut: DeviceAdapter):
    # Expected logs are sourced from nRF Desktop `sample.yaml` file.
    expected_logs = [
      "app_event_manager: e:module_state_event module:main state:READY",
      "ble_state: Bluetooth initialized",
      "settings_loader: Settings loaded",
      "ble_bond: Selected Bluetooth LE peers",
      "(ble_adv: Advertising started)|(ble_scan: Scan started)",
      "dfu: Secondary image slot is clean"
    ]
    error_log = "<err>"

    expected_regexes = list(map(re.compile, expected_logs))

    while True:
        line = dut.readline(timeout=120)

        if line is None:
            break

        assert error_log not in line

        for r in expected_regexes:
            if r.search(line):
                expected_regexes.remove(r)

        if len(expected_regexes) == 0:
            break


    # Expect to match all of the regexes
    assert len(expected_regexes) == 0


def test_boot(dut: DeviceAdapter):
    # nRF Desktop and bootloader images are already programmed at this stage.
    mcuboot_provision(dut)

    # Clear buffer to ensure proper state. Then flash and reset the board to start test. The board
    # must be programmed again at this point, because MCUboot erases application image if running
    # before KMU is provisioned.
    dut.clear_buffer()
    board_flash(dut)

    logs_verify(dut)
