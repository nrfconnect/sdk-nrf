# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
import pytest
import logging

logger = logging.getLogger(__name__)


@pytest.fixture(scope='function', autouse=True)
def test_log(request: pytest.FixtureRequest):
    logging.info("========= Test '{}' STARTED".format(request.node.nodeid))
