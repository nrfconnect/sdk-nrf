# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
import logging

import pytest
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)


def test_basic(duts: list[DeviceAdapter]):
    """Verify ESB prx and ptx samples communicate over the air.

    Boot both DUTs, check that each sample reports initialization
    complete, then confirm the prx receives packets and the ptx gets
    TX success events.
    """
    assert len(duts) > 1, "This test requires at least two DUTs, ESB PTX and ESB PRX"
    dut_rx = duts[0]
    dut_tx = duts[1]

    logger.info("Waiting for initialization complete messages...")
    lines = dut_rx.readlines_until(regex="Initialization complete", timeout=10)
    pytest.LineMatcher(lines).fnmatch_lines(
        ["*Enhanced ShockBurst prx sample", "*Initialization complete"]
    )
    lines = dut_tx.readlines_until(regex="Initialization complete", timeout=10)
    pytest.LineMatcher(lines).fnmatch_lines(
        ["*Enhanced ShockBurst ptx sample", "*Initialization complete"]
    )

    logger.info("Waiting for packets to be received and TX success events...")
    dut_rx.readlines_until(regex="Packet received", timeout=10)
    logger.info("Waiting for TX success events...")
    dut_tx.readlines_until(regex="TX SUCCESS EVENT", timeout=10)
