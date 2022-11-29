#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

from scripts.open_putty import open_putty
import sys
import subprocess

sys.path.append("./scripts")


if sys.platform == "linux":
    terminator = subprocess.Popen(
        ["terminator", "--config=scripts/linux_terminator_config"], stderr=subprocess.PIPE)
elif sys.platform == "win32":
    open_putty()
else:
    print("OS not supported")
