#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources(core.c)
zephyr_linker_sources(SECTIONS core.ld)

add_subdirectory(providers)
