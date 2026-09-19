# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from __future__ import annotations

import os
import sys
from pathlib import Path

SCRIPTS_DIR = Path(os.environ.get("ZEPHYR_BASE", "")).parent.joinpath("nrf/scripts")
if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

pytest_plugins = [
    "twister_harness.plugin",
    "twister_harness_ext.plugin",
]
