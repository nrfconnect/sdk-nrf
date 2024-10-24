#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_NRF_WIFI_BUILD_ONLY_MODE)
  return()
endif()

if(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_DISABLED)
  # The C source is same as that for built-in
  zephyr_library_sources(${ZEPHYR_BASE}/drivers/wifi/nrfwifi/src/fw_load.c)
else()
  zephyr_library_sources(ext_fw_load.c)
  if(CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP)
    # Run patches from the external flash (XIP). No need to copy.
    zephyr_code_relocate(FILES ext_fw_load.c LOCATION EXTFLASH_RODATA NOCOPY)
  endif()
endif()

if(DEFINED CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_DISABLED OR
   DEFINED CONFIG_NRF_WIFI_PATCHES_EXT_FLASH_XIP)
  # NCS doesn't rely on hal_nordic blobs for nRF70 device driver
  set(FW_BINS_BASE ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_wifi/bin/ncs)
  # RPU FW patch binaries based on the selected configuration
  if(CONFIG_NRF70_SYSTEM_MODE)
    set(NRF70_PATCH ${FW_BINS_BASE}/default/nrf70.bin)
  elseif(CONFIG_NRF70_RADIO_TEST)
    set(NRF70_PATCH ${FW_BINS_BASE}/radio_test/nrf70.bin)
  elseif(CONFIG_NRF70_SCAN_ONLY)
    set(NRF70_PATCH ${FW_BINS_BASE}/scan_only/nrf70.bin)
  elseif (CONFIG_NRF70_SYSTEM_WITH_RAW_MODES)
    set(NRF70_PATCH ${FW_BINS_BASE}/system_with_raw/nrf70.bin)
  elseif(CONFIG_NRF70_OFFLOADED_RAW_TX)
    set(NRF70_PATCH ${FW_BINS_BASE}/offloaded_raw_tx/nrf70.bin)
  else()
    # Error
    message(FATAL_ERROR "Unsupported nRF70 patch configuration")
  endif()

  set(gen_inc_dir ${ZEPHYR_BINARY_DIR}/misc/generated)
  zephyr_include_directories(${gen_inc_dir})
  set(gen_dir ${gen_inc_dir}/nrf70_fw_patch)
  generate_inc_file_for_target(
    nrfwifi
    ${NRF70_PATCH}
    ${gen_dir}/nrf70.bin.inc
  )
endif()
