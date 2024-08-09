# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import sys
import psutil
from signal import SIGINT
from subprocess import Popen, PIPE

# For example to send Ctrl-C to ninja build when its outputs "syncing doxygen output":
#   python interrupt_on.py "syncing doxygen output" ninja nrf

def run_interrupted(match_text: str, command: 'list[str]'):
    print('Run', command)
    print('    and interrupt on:', match_text)
    match_text = match_text.lower()
    p = Popen(command, stdout=PIPE, stderr=None, encoding="utf-8")
    interrupted = False
    while True:
        line = p.stdout.readline()
        if line:
            print(line, end='')
            if line.lower().find(match_text) >= 0:
                print('Sending SIGINT signal.')
                parent = psutil.Process(p.pid)
                for child in parent.children(recursive=True):
                    child.send_signal(SIGINT)
                parent.send_signal(SIGINT)
                interrupted = True
        if p.poll() is not None:
            break
    if interrupted:
        print('Correctly interrupted.')
    elif p.returncode:
        print(f'Failed with return code {p.returncode}.')
        sys.exit(p.returncode)
    else:
        print('Build not interrupted. You may experience long building time.')

if len(sys.argv) <= 2:
    print(f'Usage: {sys.argv[0]} "Matching string" command parameters...')
    sys.exit(1)

run_interrupted(sys.argv[1], sys.argv[2:])
