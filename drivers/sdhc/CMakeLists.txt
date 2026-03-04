#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_amend()

if(CONFIG_SDHC_NRF_SEMMC)
  set(SP_DIR ${ZEPHYR_NRFXLIB_MODULE_DIR}/softperipheral)
  set(SEMMC_DIR ${SP_DIR}/sEMMC)

  dt_comp_path(semmc_path COMPATIBLE "nordic,nrf-semmc" IDX 0)
  dt_reg_addr(semmc_addr PATH ${semmc_path})
  if(CONFIG_SOC_SERIES_NRF54H)
    math(EXPR semmc_sp_firmware_addr "${semmc_addr} - 0x3b40")
  elseif(CONFIG_SOC_SERIES_NRF54L)
    math(EXPR semmc_sp_firmware_addr "${semmc_addr} - 0x4200")
  else()
    message(FATAL_ERROR "Unsupported SoC series")
  endif()

  zephyr_library_compile_definitions(
    NRF_SEMMC_ENABLED=1
    NRF_SEMMC_SP_FIRMWARE_ADDR=${semmc_sp_firmware_addr}
  )
  zephyr_library_include_directories(
    ${SP_DIR}/include
    ${SEMMC_DIR}/include
    ${SEMMC_DIR}/include/${CONFIG_SOC_SERIES}
  )
  zephyr_library_sources(
    ${SEMMC_DIR}/src/nrf_semmc.c
    sdhc_semmc.c
  )
endif()
