# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Helper functions and decorators for test utilities and board management."""

from __future__ import annotations

import functools
import logging
import os
import re
import shlex
import subprocess
import time

logger = logging.getLogger(__name__)


def retry(retry_num: int):
    """Retry the function if it raises an exception.

    Retries the decorated function up to `retry_num` times if an exception is raised.
    """

    def decorator_retry(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            attempt = 0
            while True:
                try:
                    return func(*args, **kwargs)
                except Exception:
                    if attempt < retry_num:
                        attempt += 1
                        logging.exception("Exception occurred in %s", func)
                        time.sleep(1)
                        logging.info(f"Trying attempt {attempt}/{retry_num}")
                    else:
                        raise

        return wrapper

    return decorator_retry


def timer(func):
    """Measure execution time of the function."""

    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        start_time = time.time()
        result = func(*args, **kwargs)
        end_time = time.time()
        execution_time = end_time - start_time
        logger.debug(f"Execution time of {func.__name__}: {execution_time:.3f} seconds")
        return result

    return wrapper


def normalize_path(path: str | None) -> str | None:
    """Normalize and expand a filesystem path."""
    if path is not None:
        path = os.path.expanduser(os.path.expandvars(path))
        path = os.path.normpath(os.path.abspath(path))
    return path


def run_command(command: list[str], timeout: int = 30) -> subprocess.CompletedProcess:
    """Run a shell command with logging and error handling."""
    logger.info(f"CMD: {shlex.join(command)}")
    ret: subprocess.CompletedProcess = subprocess.run(
        command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout
    )
    if ret.returncode:
        logger.error(f"Failed command: {shlex.join(command)}")
        logger.info(ret.stdout)
        raise subprocess.CalledProcessError(ret.returncode, command)
    return ret


def nrfutil_write(
    address: str, value: str, dev_id: str | None = None, core: str | None = None
) -> None:
    """Write a value to a specific address on the device.

    :param address: memory address to write to
    :param value: value to write
    :param dev_id: serial number of the device
    :param core: core to target (optional)
    """
    cmd = [
        "nrfutil",
        "device",
        "write",
        "--address",
        address,
        "--value",
        value,
    ]

    if dev_id:
        cmd.extend(["--serial-number", dev_id])
    if core:
        cmd.extend(["--core", core])

    run_command(cmd)


def nrfutil_read(
    address: str | int, bytes_count: int, dev_id: str | None = None, core: str | None = None
) -> str:
    """Read data from a specific address on the device.

    :param address: memory address to read from (can be hex string like "0x1000" or int)
    :param bytes_count: number of bytes to read
    :param dev_id: serial number of the device
    :param core: core to target (optional)
    :return: the data read from the device as a hex string (e.g., "0x12345678")
    """
    cmd = [
        "nrfutil",
        "device",
        "read",
        "--address",
        str(address),
        "--bytes",
        str(bytes_count),
    ]

    if dev_id:
        cmd.extend(["--serial-number", dev_id])
    if core:
        cmd.extend(["--core", core])

    result = run_command(cmd)
    logger.debug(f"nrfutil read output: {result.stdout}")

    # Parse the address from the input to create the regex pattern
    if isinstance(address, str) and address.startswith("0x"):
        addr_int = int(address, 16)
    else:
        addr_int = int(address, 0)  # Handle both hex and decimal  # type: ignore

    # Create regex pattern to match the output format: "0xADDRESS: DATA"
    hex_pattern = rf"0x{addr_int:08X}:\s+(\S+)"
    match = re.search(hex_pattern, result.stdout, re.IGNORECASE)

    if match:
        hex_data = f"0x{match.group(1).lower()}"
        logger.info(f"Parsed hex data: {hex_data}")
        return hex_data
    else:
        raise ValueError(f"Could not parse hex data from nrfutil output: {result.stdout}")
