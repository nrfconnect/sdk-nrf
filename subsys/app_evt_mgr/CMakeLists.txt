#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
zephyr_sources(app_evt_mgr.c)
zephyr_sources_ifdef(CONFIG_SHELL app_evt_mgr_shell.c)

zephyr_linker_sources(SECTIONS aem.ld)
