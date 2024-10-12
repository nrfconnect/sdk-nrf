#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_DISABLED)
  zephyr_library_sources(ext_fw_load.c)
  if(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP)
    # Run patches from the external flash (XIP). No need to copy.
    zephyr_code_relocate(FILES ext_fw_load.c LOCATION EXTFLASH_RODATA NOCOPY)
  endif()
endif()
