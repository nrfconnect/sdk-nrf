# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
import pytest
import logging

from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.fixtures import determine_scope

logger = logging.getLogger(__name__)


@pytest.fixture(scope='function', autouse=True)
def test_log(request: pytest.FixtureRequest):
    logging.info("========= Test '{}' STARTED".format(request.node.nodeid))


@pytest.fixture(scope=determine_scope)
def no_reset(device_object: DeviceAdapter):
    """Do not reset after flashing."""
    device_object.device_config.west_flash_extra_args.append("--no-reset")
    yield
    device_object.device_config.west_flash_extra_args.remove("--no-reset")
