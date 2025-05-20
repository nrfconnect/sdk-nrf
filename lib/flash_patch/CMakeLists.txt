#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_FLASH_PATCH_WARN)
  message(WARNING "
      ----------------------------------------------------------
      --- WARNING: To maintain the integrity of secure boot, ---
      --- enable CONFIG_DISABLE_FLASH_PATCH in production.   ---
      ----------------------------------------------------------"
  )
endif()

if (CONFIG_DISABLE_FLASH_PATCH)
  zephyr_sources(flash_patch.c)
endif()
