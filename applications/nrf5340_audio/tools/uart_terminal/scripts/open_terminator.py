#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import sys
import subprocess
from get_serial_ports import get_serial_ports

ports = get_serial_ports()

if int(sys.argv[1]) < len(ports):
    subprocess.Popen(["minicom", "--color=on", "-b 115200", "-8", "-D " + ports[int(sys.argv[1])]])
else:
    print("Not enough boards connected")
