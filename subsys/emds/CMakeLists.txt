#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources(emds.c)
zephyr_sources(emds_flash.c)
zephyr_linker_sources(SECTIONS emds_types.ld)
