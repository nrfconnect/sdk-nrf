#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_CODE_EXTENDER src/code_extender.c)
zephyr_linker_sources_ifdef(CONFIG_CODE_EXTENDER SECTIONS code_extender.ld)
