# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""TF-M regression tests on devices that must be provisioned first.

The nRF54L board configurations for this test enable
CONFIG_TFM_NRF_PROVISIONING, so TF-M refuses to boot ("Invalid LCS") until the
device has been provisioned: the CRACEN IKG seed, from which the Initial
Attestation Key is derived, has to be written and the PSA lifecycle state
advanced out of Device Assembly and Test. Provisioning is a separate image that
has to run before the test image, which the console harness cannot do, so the
sequence is driven from here:

1. Recover the device. A previous run leaves it SECURED with the debug signals
   locked, and the provisioning image only runs in the Assembly and Test state.
2. Program and run the provisioning image, built alongside this test as a
   build-only sysbuild image (SB_CONFIG_TEST_PROVISIONING_IMAGE).
3. Program the test image, keeping the UICR and the KMU. TF-M advances the
   lifecycle state to SECURED on the first boot, locks the debug signals and
   resets, and the test suites then run.
"""

from __future__ import annotations

import logging
import re
import time
from pathlib import Path

import pytest
from twister_harness import DeviceAdapter
from twister_harness.fixtures import determine_scope
from twister_harness_ext.utils.common import run_command

logger = logging.getLogger(__name__)

# The harness_config regexes of the console-driven scenarios, plus the Initial
# Attestation suite, which is the reason this test needs provisioning.
EXPECTED_SUITES = [
    r"Test suite 'PSA protected storage NS interface tests.*' has.*PASSED",
    r"Test suite 'PSA internal trusted storage NS interface tests.*' has.*PASSED",
    r"Test suite 'Crypto non-secure interface test.*' has.*PASSED",
    r"Test suite 'Platform Service Non-Secure interface tests.*' has.*PASSED",
    r"Test suite 'Initial Attestation Service non-secure interface tests.*' has.*PASSED",
]

FAILED_SUITE_REGEX = re.compile(r"Test suite '.*' has.*FAILED")

# Recovering and programming the merged TF-M image both take longer than the
# 30 s default of run_command().
NRFUTIL_TIMEOUT = 180

# The secure test suites run before the non-secure ones.
TEST_OUTPUT_TIMEOUT = 240.0


def _nrfutil_device(args: list[str], dev_id: str) -> None:
    run_command(["nrfutil", "device", *args, "--serial-number", dev_id], timeout=NRFUTIL_TIMEOUT)


def _program_and_reset(firmware: Path, dev_id: str, erase_mode: str) -> None:
    """Program an image, resetting as a post-program action of the same command.

    Resetting separately would reset the device while the debugger session used
    for programming is still being torn down.
    """
    _nrfutil_device(
        [
            "program",
            "--firmware",
            str(firmware),
            "--options",
            f"chip_erase_mode={erase_mode},reset=RESET_PIN",
        ],
        dev_id,
    )


def _reconnect(dut: DeviceAdapter, timeout: float = 30.0) -> None:
    """Re-open the serial connection, waiting for the port to come back.

    Programming opens and closes a debugger session, which can take the board's
    USB serial port with it.
    """
    dut.disconnect()
    deadline = time.time() + timeout
    while True:
        try:
            dut.connect()
            return
        except Exception:  # noqa: BLE001 - any port error is worth retrying here
            if time.time() >= deadline:
                raise
            time.sleep(0.5)


@pytest.fixture(scope=determine_scope)
def provisioned_dut(unlaunched_dut: DeviceAdapter) -> DeviceAdapter:
    """Provision the device, then program the test image and let it boot.

    Programming is done with nrfutil rather than by letting the device adapter
    flash the test image, because the two images have to be programmed in order
    and with different erase modes.
    """
    dut = unlaunched_dut
    dev_id = dut.device_config.id
    if not dev_id:
        pytest.fail("No device ID available; provide one through the hardware map.")

    build_dir = Path(dut.device_config.current_build_dir or dut.device_config.build_dir)
    provisioning_hex = build_dir / "provisioning_image" / "zephyr" / "zephyr.hex"
    test_hex = Path(dut.device_config.app_build_dir) / "zephyr" / "tfm_merged.hex"

    for hex_file in (provisioning_hex, test_hex):
        if not hex_file.exists():
            pytest.fail(f"Expected image not found: {hex_file}")

    logger.info("Recovering %s to the Assembly and Test lifecycle state", dev_id)
    _nrfutil_device(["recover"], dev_id)

    # Connect before programming so that no boot output is lost.
    dut.start_reader()
    dut.connect()

    logger.info("Provisioning the device with %s", provisioning_hex)
    _program_and_reset(provisioning_hex, dev_id, "ERASE_ALL")
    lines = dut.readlines_until(regex="provisioning_image: (Success!|Failure)", timeout=60.0)
    if not any("Success!" in line for line in lines):
        pytest.fail("Provisioning did not succeed; see the log above.")

    # ERASE_ALL here would erase the UICR and the KMU and undo the provisioning.
    logger.info("Programming the test image %s", test_hex)
    _program_and_reset(test_hex, dev_id, "ERASE_RANGES_TOUCHED_BY_FIRMWARE")
    _reconnect(dut)

    yield dut

    dut.close()


def test_tfm_regression_on_provisioned_device(provisioned_dut: DeviceAdapter):
    """Verify that the TF-M regression suites pass on a provisioned device."""
    dut = provisioned_dut

    # The suites are reported in order, so waiting for the last one collects the
    # output that the assertions below need.
    lines = dut.readlines_until(regex=EXPECTED_SUITES[-1], timeout=TEST_OUTPUT_TIMEOUT)

    failed = [line for line in lines if FAILED_SUITE_REGEX.search(line)]
    assert not failed, f"Test suites reported as FAILED: {failed}"

    for expected in EXPECTED_SUITES:
        regex = re.compile(expected)
        assert any(regex.search(line) for line in lines), f"Missing expected output: {expected}"
