#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_amend()

if(CONFIG_SDHC_NRF_SEMMC)
  set(sp_dir ${ZEPHYR_NRFXLIB_MODULE_DIR}/softperipheral)
  set(semmc_dir ${sp_dir}/sEMMC)

  zephyr_library_compile_definitions(
    NRF_SEMMC_ENABLED=1
  )
  zephyr_library_include_directories(
    ${sp_dir}/include
    ${semmc_dir}/include
    ${semmc_dir}/include/${CONFIG_SOC_SERIES}
  )
  zephyr_library_sources(
    ${semmc_dir}/src/nrf_semmc.c
    sdhc_semmc.c
  )
endif()
