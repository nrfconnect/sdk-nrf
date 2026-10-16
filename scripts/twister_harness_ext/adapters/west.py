# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from __future__ import annotations

from pathlib import Path

from twister_harness_ext.adapters.cmd import run_command


def flash_board(build_dir: Path | str, dev_id: str | None, erase: bool = False):
    command = ['west', 'flash', '--skip-rebuild', '-d', str(build_dir)]
    if dev_id:
        command.extend(['--dev-id', dev_id])
    if erase:
        command.extend(['--erase'])
    run_command(command)
