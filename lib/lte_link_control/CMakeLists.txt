#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(lte_lc.c)
zephyr_library_sources(lte_lc_helpers.c)
zephyr_library_sources(lte_lc_modem_hooks.c)
zephyr_library_sources_ifdef(CONFIG_LTE_LC_TRACE lte_lc_trace.c)
zephyr_library_sources_ifdef(CONFIG_LTE_SHELL lte_lc_shell.c)

zephyr_linker_sources(RODATA lte_lc.ld)
