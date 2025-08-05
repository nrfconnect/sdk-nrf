#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_amend()
zephyr_library_sources_ifdef(CONFIG_MSPI_HPF mspi_hpf.c)

if(CONFIG_MSPI_NRF_SQSPI)
  set(SP_DIR ${ZEPHYR_NRFXLIB_MODULE_DIR}/softperipheral)
  set(SQSPI_DIR ${SP_DIR}/sQSPI)

  dt_comp_path(sqspi_path COMPATIBLE "nordic,nrf-sqspi" IDX 0)
  dt_reg_addr(sqspi_addr PATH ${sqspi_path})
  math(EXPR sqspi_sp_firmware_addr "${sqspi_addr} - 0x3b40")

  zephyr_library_compile_definitions(
    NRF_SQSPI_ENABLED=1
    NRF_SQSPI_MAX_NUM_DATA_LINES=4
    NRF_SQSPI_SP_FIRMWARE_ADDR=${sqspi_sp_firmware_addr}
  )
  zephyr_library_include_directories(
    ${SP_DIR}/include
    ${SQSPI_DIR}/include
    ${SQSPI_DIR}/include/${CONFIG_SOC}
  )
  zephyr_library_sources(
    ${SQSPI_DIR}/src/nrf_sqspi.c
    mspi_sqspi.c
  )
endif()
