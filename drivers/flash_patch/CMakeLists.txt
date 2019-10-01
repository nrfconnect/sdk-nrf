#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if((NOT CONFIG_DISABLE_FLASH_PATCH) AND CONFIG_SOC_NRF52840
    AND (CONFIG_IS_SECURE_BOOTLOADER OR CONFIG_MCUBOOT))
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
