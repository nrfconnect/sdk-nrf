#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(
  at_shell.c
  at_shell_common.c
)

zephyr_library_sources_ifdef(CONFIG_AT_SHELL_CMD_MODE at_cmd_mode.c)
