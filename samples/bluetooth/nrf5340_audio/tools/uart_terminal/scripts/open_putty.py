#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import subprocess
from get_serial_ports import get_serial_ports


def open_putty():
    ports = get_serial_ports()

    for port in ports:
        subprocess.Popen("putty -serial " + port + " -sercfg 115200,8,n,1,N")
