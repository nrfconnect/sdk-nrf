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

  zephyr_library_compile_definitions(
    NRFX_QSPI2_ENABLED=1
    NRFX_QSPI2_MAX_NUM_DATA_LINES=4
  )
  zephyr_library_include_directories(
    ${SP_DIR}/include
    ${SQSPI_DIR}/include
  )
  zephyr_library_include_directories_ifdef(CONFIG_SOC_NRF54L15
    ${SQSPI_DIR}/include/nrf54l15
  )
  zephyr_library_include_directories_ifdef(CONFIG_SOC_NRF54H20
    ${SQSPI_DIR}/include/nrf54h20
  )
  zephyr_library_sources(
    ${SQSPI_DIR}/src/nrfx_qspi2.c
    mspi_sqspi.c
  )
endif()
