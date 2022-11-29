#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import sys
import subprocess

def get_serial_ports():
    nrfjprog_com = subprocess.Popen(["nrfjprog", "--com"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    nrfjprog_com.wait()

    if nrfjprog_com.returncode != 0:
        sys.exit("'nrfjprog --com' failed")

    output = nrfjprog_com.communicate()
    output_decoded = output[0].decode()
    output_decoded_lines = output_decoded.splitlines()

    ports = list()

    for line in output_decoded_lines:
        if "VCOM0" in line:
            info = line.split("    ")
            ports.append(info[1])

    return ports
