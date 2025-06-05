#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_NEW_EMDS)
  zephyr_sources(emds_new.c)
  zephyr_sources(emds_flash_new.c)
else()
  zephyr_sources(emds.c)
  zephyr_sources(emds_flash.c)
endif()
zephyr_linker_sources(SECTIONS emds_types.ld)
