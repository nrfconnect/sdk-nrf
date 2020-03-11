#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(.)
zephyr_sources(event_manager.c)
zephyr_sources_ifdef(CONFIG_SHELL event_manager_shell.c)
