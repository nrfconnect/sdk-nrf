#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import sys
import subprocess
import os

absolutePath = os.path.join(os.path.abspath(os.getcwd()), "scripts")
sys.path.append(absolutePath)
from open_putty import open_putty

print("Configure and open terminal(s)\n")

if sys.platform == "linux":
    terminator = subprocess.Popen(
        ["terminator", "--config=scripts/linux_terminator_config"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
elif sys.platform == "win32":
    open_putty()
else:
    print("OS not supported")

print("\nScript finished\n")
