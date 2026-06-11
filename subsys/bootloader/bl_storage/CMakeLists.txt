#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(bl_storage.c)

if(CONFIG_TRUSTED_EXECUTION_NONSECURE)
  zephyr_get(tfm_bl_storage_addr SYSBUILD LOCAL VAR TFM_BL_STORAGE_ADDR)

  if(tfm_bl_storage_addr AND "${tfm_bl_storage_addr}" GREATER "0")
    zephyr_library_compile_definitions(TFM_BL_STORAGE_ADDR=${tfm_bl_storage_addr})
  endif()
endif()

if(CONFIG_MCUBOOT_HW_DOWNGRADE_PREVENTION)
  # NB: This option can only be enabled for the MCUBoot image
  zephyr_library_sources(nrf_nv_counters.c)
endif()
