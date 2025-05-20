#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
target_include_directories(app PRIVATE .)

target_sources(app PRIVATE at_shell.c)
target_sources_ifdef(CONFIG_MOSH_AT_CMD_MODE app PRIVATE at_cmd_mode.c)
target_sources_ifdef(CONFIG_MOSH_AT_CMD_MODE app PRIVATE at_cmd_mode_sett.c)
target_sources_ifdef(CONFIG_MOSH_AT_CMD_MODE app PRIVATE at_cmd_mode_custom.c)
