# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""Tests wrapping twister tests that need provisioned KMU keys."""

from __future__ import annotations

import logging
import os
from pathlib import Path

import pytest
from helpers import reset_board
from key_provisioning import provision_keys_for_kmu
from twister_harness import DeviceAdapter
from twister_harness.fixtures import determine_scope
from twister_harness.helpers.utils import match_lines

logger = logging.getLogger(__name__)

ZEPHYR_BASE = os.environ["ZEPHYR_BASE"]
APP_KEYS_FOR_KMU_LOCK_TEST = Path(
    f"{ZEPHYR_BASE}/../nrf/tests/subsys/bootloader/boot_lock_kmu_keys"
).resolve()


@pytest.fixture(scope=determine_scope)
def kmu_provision_all(no_reset, dut: DeviceAdapter):
    """Provision all KMU key slots using west ncs-provision upload command."""

    key_file = APP_KEYS_FOR_KMU_LOCK_TEST / "private_key.pem"

    # The maximum number of key slots per type (BL_PUBKEY, UROT_PUBKEY)
    MAX_KEY_SLOTS_PER_TYPE = 3
    all_keys = [key_file] * MAX_KEY_SLOTS_PER_TYPE

    # Provision the same key to all KMU key slots.
    # It does not matter for the tests if the keys differ.
    provision_keys_for_kmu(all_keys, keyname="BL_PUBKEY", dev_id=dut.device_config.id)
    provision_keys_for_kmu(all_keys, keyname="UROT_PUBKEY", dev_id=dut.device_config.id)

    reset_board(dut.device_config.id)


@pytest.mark.usefixtures("kmu_provision_all")
def test_passes_after_provisioning_all_keys(dut: DeviceAdapter):
    """Verify that a twister test passes after provisioning all KMU key slots.

    This test provisions all KMU key slots and verifies that a test passes.
    """

    success_str = "PROJECT EXECUTION SUCCESSFUL"
    failed_str = "PROJECT EXECUTION FAILED"
    regex = f"{success_str}|{failed_str}"

    output = dut.readlines_until(regex=regex, timeout=120)
    match_lines(output, [success_str])
