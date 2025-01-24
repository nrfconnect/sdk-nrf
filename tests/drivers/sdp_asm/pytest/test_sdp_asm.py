#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import logging
import os
import subprocess

from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


def execute_shell_cmd(cmd, work_dir: str):
    os.chdir(work_dir)
    try:
        logger.info(f"Executing:\n{cmd}")
        proc = subprocess.run(
            cmd, shell=False,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        logger.info(f'out: {proc.stdout}\n')
        logger.info(f'err: {proc.stderr}\n')
    except OSError as exc:
        logger.error(f"Command has failed:\n{cmd}\n{exc}")


def test_sdp_asm(dut: DeviceAdapter):
    BUILD_DIR = str(dut.device_config.build_dir)
    build_log_file = f"{BUILD_DIR}/build.log"
    OUTPUT1 = """initial: 0, 0, 0
after add_1: 1, 2, 3
after add_10: 11, 22, 33
after add_100: 111, 222, 333"""
    OUTPUT2 = """initial: 0, 0, 0
after add_1: 2, 3, 4
after add_10: 12, 23, 34
after add_100: 212, 323, 434"""

    logger.info("# Get output from serial port")
    output = "\n".join(dut.readlines_until(regex='Test completed', timeout=5.0))

    logger.info("# Confirm that precompiled .S files were linked in the build")
    assert OUTPUT1 in output, f"Unexpected output: {output}"

    logger.info("# Confirm that build log contains warnings")
    with open(f"{build_log_file}", errors="ignore") as log_file:
        log_file_content = log_file.read()
        assert "add_1.s ASM file content has changed" in log_file_content
        assert "File add_10.s has not changed" in log_file_content
        assert "add_100.s ASM file content has changed" in log_file_content

    for _ in range(2):
        logger.info("# Install newly compiled .S files")
        cmd = ['ninja', 'asm_install']
        execute_shell_cmd(cmd, f"{BUILD_DIR}/sdp_asm")

        logger.info("# Recompile")
        cmd = ['ninja']
        execute_shell_cmd(cmd, f"{BUILD_DIR}")

    logger.info("# Reflash")
    dut._flash_and_run()

    logger.info("# Get output from serial port")
    output = "\n".join(dut.readlines_until(regex='Test completed', timeout=5.0))

    logger.info("# Confirm that new .S files were linked in the build")
    assert OUTPUT2 in output, f"Unexpected output: {output}"
