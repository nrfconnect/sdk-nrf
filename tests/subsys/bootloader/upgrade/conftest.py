# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Pytest configuration, hooks, and fixtures for MCUboot test suite."""

import os
import sys
from pathlib import Path

# Add the directory to PYTHONPATH
zephyr_base = os.getenv("ZEPHYR_BASE")
if zephyr_base:
    sys.path.insert(
        0, os.path.join(zephyr_base, "scripts", "pylib", "pytest-twister-harness", "src")
    )
else:
    raise OSError("ZEPHYR_BASE environment variable is not set")

SCRIPTS_DIR = Path(zephyr_base).parent.joinpath("nrf/scripts")
if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

pytest_plugins = [
    "twister_harness.plugin",
    "twister_harness_ext.plugin",
]
