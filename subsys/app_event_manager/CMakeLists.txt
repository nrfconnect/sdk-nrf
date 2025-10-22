#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
zephyr_sources(app_event_manager.c)
zephyr_sources_ifdef(CONFIG_APP_EVENT_MANAGER_SHELL app_event_manager_shell.c)

zephyr_linker_sources(SECTIONS aem.ld)
