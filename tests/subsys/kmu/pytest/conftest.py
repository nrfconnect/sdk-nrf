# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import os
import sys
from pathlib import Path

import pytest
from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.fixtures import determine_scope

SCRIPTS_DIR = Path(os.environ.get("ZEPHYR_BASE", "")).parent.joinpath("nrf/scripts")
if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

pytest_plugins = [
    "twister_harness.plugin",
    "twister_harness_ext.plugin",
]


@pytest.fixture(scope=determine_scope)
def no_reset(device_object: DeviceAdapter):
    """Do not reset after flashing."""
    device_object.device_config.west_flash_extra_args.append("--no-reset")
    yield
    device_object.device_config.west_flash_extra_args.remove("--no-reset")
