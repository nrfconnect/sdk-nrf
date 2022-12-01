#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

from scripts.open_putty import open_putty
import sys
import subprocess

print("Configure and open terminal(s)\n")

if sys.platform == "linux":
    terminator = subprocess.Popen(
        ["terminator", "--config=scripts/linux_terminator_config"], stderr=subprocess.PIPE)

    if terminator.returncode == None:
        print("No nRF5340 audio DKs found\n")
elif sys.platform == "win32":
    open_putty()
else:
    print("OS not supported")

print("\nScript finished\n")
