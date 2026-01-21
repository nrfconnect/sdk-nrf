# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Pytest configuration, hooks, and fixtures for MCUboot test suite."""

import logging

import pytest
from helpers import reset_board
from key_provisioning import provision_mcuboot, provision_nsib
from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.fixtures import determine_scope

logger = logging.getLogger(__name__)


@pytest.fixture(scope=determine_scope)
def no_reset(device_object: DeviceAdapter):
    """Do not reset after flashing."""
    device_object.device_config.west_flash_extra_args.append("--no-reset")
    yield
    device_object.device_config.west_flash_extra_args.remove("--no-reset")
    # Reset the command list to avoid side effects on other tests
    device_object.command = []


@pytest.fixture(scope=determine_scope)
def kmu_provision(no_reset, dut: DeviceAdapter):  # noqa: ARG001
    """Provision KMU keys using west ncs-provision upload command."""
    # only for sysbuild
    if dut.device_config.build_dir != dut.device_config.app_build_dir:
        provision_nsib(dut)
        provision_mcuboot(dut)
    reset_board(dut.device_config.id)
