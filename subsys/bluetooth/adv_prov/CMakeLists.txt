#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources(core.c)
zephyr_linker_sources(SECTIONS core.ld)
zephyr_iterable_section(NAME bt_le_adv_prov_ad KVMA RAM_REGION GROUP RODATA_REGION)
zephyr_iterable_section(NAME bt_le_adv_prov_sd KVMA RAM_REGION GROUP RODATA_REGION)

add_subdirectory(providers)
