#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources(event_manager_proxy.c)

# Note:
# Linker script file for the em_proxy has to be placed after the content of the em.ld file
# from event_manager module that has SORT_KEY set to 'default'.
zephyr_linker_sources(RAM_SECTIONS em_proxy.ld SORT_KEY 'zzz_place_after_em_ld_file')
