#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_amend()

if(CONFIG_CAN_NRF_SCAN)
  set(sp_dir ${ZEPHYR_NRFXLIB_MODULE_DIR}/softperipheral)
  set(scan_dir ${sp_dir}/sCAN)

  zephyr_library_compile_definitions(
    NRF_SCAN_ENABLED=1
  )

  zephyr_library_include_directories(
    ${sp_dir}/include
    ${scan_dir}/include
    ${scan_dir}/include/${CONFIG_SOC_SERIES}
  )

  zephyr_library_sources(
    ${scan_dir}/src/nrf_scan.c
    can_nrf_scan.c
  )
endif()
