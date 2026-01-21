#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause


from dataclasses import dataclass

LEVELS = {"issue": "issue", "warning": "warning", "info": "info", "debug": "debug"}


@dataclass
class MatterSampleCheckerResult:
    level: str = LEVELS["info"]
    message: str = ""
