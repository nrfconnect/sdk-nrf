# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import os
from pathlib import Path

ZEPHYR_BASE = os.environ["ZEPHYR_BASE"]
APP_KEYS_FOR_KMU = Path(f"{ZEPHYR_BASE}/../nrf/tests/subsys/kmu/keys").resolve()
