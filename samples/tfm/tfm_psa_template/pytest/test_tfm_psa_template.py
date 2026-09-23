# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""TF-M PSA template on a device that is provisioned first.

The sample enables CONFIG_TFM_NRF_PROVISIONING, so TF-M refuses to boot
("Invalid LCS") until the provisioning_image sample has written key material
and advanced the PSA lifecycle state out of Device Assembly and Test. That
has to happen, in the order the sample's README describes, before the
application is flashed - which a console harness can't drive, hence pytest.
"""

from __future__ import annotations

import logging
import os
import re
from pathlib import Path

import pytest
from twister_harness import DeviceAdapter
from twister_harness.fixtures import determine_scope
from twister_harness.helpers.utils import find_in_config
from twister_harness_ext.utils.common import run_command
from twister_harness_ext.utils.required_build import get_required_build

logger = logging.getLogger(__name__)

PROVISIONING_IMAGE_SOURCE = (
    Path(os.environ["ZEPHYR_BASE"]).parent / "nrf" / "samples" / "tfm" / "provisioning_image"
)

# Recovering and programming every domain of the nRF5340 build both take longer
# than the 30 s default of run_command().
FLASH_TIMEOUT = 60

PROVISIONING_BUILD_TIMEOUT = 180

PROVISIONING_TIMEOUT = 20.0

# TF-M finishes provisioning and resets before the application starts.
BOOT_TIMEOUT = 30.0


def _west_flash(build_dir: Path, dev_id: str, extra_args: list[str] | None = None) -> None:
    command = ["west", "flash", "--skip-rebuild", "-d", str(build_dir), "--dev-id", dev_id]
    run_command(command + (extra_args or []), timeout=FLASH_TIMEOUT)


def _secure_board(dut: DeviceAdapter) -> str:
    """Return the secure board target, for example nrf9160dk@0.14.0/nrf9160."""
    board = dut.device_config.platform
    if not board.endswith("/ns"):
        pytest.fail(f"Expected a non-secure board target, got: {board}")
    return board.removesuffix("/ns")


def _provision(dut: DeviceAdapter, build_dir: Path, dev_id: str) -> None:
    sysbuild_config = build_dir / "zephyr" / ".config"
    if find_in_config(sysbuild_config, "SB_CONFIG_SAMPLE_PROVISIONING_IMAGE") == "y":
        logger.info("Provisioning with the bundled provisioning_image domain")
        _west_flash(build_dir, dev_id, ["--domain", "provisioning_image", "--recover"])
        return

    provisioning_build_dir = get_required_build(
        dut,
        source_dir=PROVISIONING_IMAGE_SOURCE,
        testsuite="sample.tfm.provisioning_image",
        board=_secure_board(dut),
        timeout=PROVISIONING_BUILD_TIMEOUT,
    )
    logger.info("Recovering %s and provisioning with %s", dev_id, provisioning_build_dir)
    _west_flash(provisioning_build_dir, dev_id, ["--recover"])


@pytest.fixture(scope=determine_scope)
def provisioned_dut(unlaunched_dut: DeviceAdapter) -> DeviceAdapter:
    """Provision the device, then program the application and let it boot.

    Programming is done with west rather than by letting the device adapter
    flash the application, because the provisioning image has to be programmed
    and run first.
    """
    dut = unlaunched_dut
    dev_id = dut.device_config.id
    if not dev_id:
        pytest.fail("No device ID available; provide one through the hardware map.")

    build_dir = Path(dut.device_config.current_build_dir or dut.device_config.build_dir)

    # Connect before programming so that no boot output is lost.
    dut.start_reader()
    dut.connect()

    _provision(dut, build_dir, dev_id)
    lines = dut.readlines_until(regex="(Success!|Failure)", timeout=PROVISIONING_TIMEOUT)
    if not any("Success!" in line for line in lines):
        pytest.fail("Provisioning did not succeed; see the log above.")

    # --erase or --recover here would wipe the provisioned keys and lifecycle state.
    logger.info("Programming the application from %s", build_dir)
    dut.clear_buffer()
    _west_flash(build_dir, dev_id)

    yield dut

    dut.close()


def test_tfm_psa_template_attests_on_provisioned_device(provisioned_dut: DeviceAdapter):
    """Verify that the application gets an attestation token from TF-M."""
    received_regex = r"Received initial attestation token of [0-9]+ bytes\."
    lines = provisioned_dut.readlines_until(regex=received_regex, timeout=BOOT_TIMEOUT)

    requesting_regex = r"Requesting initial attestation token with [0-9]+ byte challenge\."
    assert any(re.search(requesting_regex, line) for line in lines), "Missing the request line"
