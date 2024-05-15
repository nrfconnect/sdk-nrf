# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import os
import sys
from pathlib import Path
from jinja2 import Template


def warning(*args, **kwargs):
    args = ('\x1B[33mwarning:\x1B[0m', *args)
    print(*args, **kwargs, file=sys.stderr)


def error(*args, **kwargs):
    args = ('\x1B[31merror:\x1B[0m', *args)
    print(*args, **kwargs, file=sys.stderr)


def compile_messages(messages):
    result = {}
    for key in messages.keys():
        result[key] = Template(messages[key])
    return result


def find_devicetree_sources() -> 'str|None':
    sources = None
    zephyr_base = os.getenv('ZEPHYR_BASE')
    if zephyr_base is not None:
        zephyr_base = Path(zephyr_base)
        sources = zephyr_base / 'scripts/dts/python-devicetree/src'
        if sources.exists():
            return str(sources)
    west_root = Path(__file__).parent.parent.absolute()
    for _i in range(0, 6):
        sources = west_root / 'zephyr/scripts/dts/python-devicetree/src'
        if sources.exists():
            return str(sources)
        west_root = west_root.parent
    return None

devicetree_sources = find_devicetree_sources()
