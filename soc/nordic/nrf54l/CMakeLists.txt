# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library_sources(
  ${ZEPHYR_BASE}/soc/nordic/nrf54l/soc.c
  ${ZEPHYR_BASE}/soc/nordic/validate_rram_partitions.c
  )

# We need a buffer in memory in a static location which can be used by
# the KMU peripheral. The KMU has a static destination address, we chose
# this address to be 0x20000000, which is the first address in the SRAM.
if(NOT CONFIG_BUILD_WITH_TFM AND CONFIG_PSA_NEED_CRACEN_KMU_DRIVER AND CONFIG_XIP)
  # Exclamation mark is printable character with the lowest number in ASCII table.
  # We are sure that this file will be included first.
  zephyr_linker_sources(RAM_SECTIONS SORT_KEY ! ${ZEPHYR_BASE}/soc/nordic/nrf54l/kmu_push_area_section.ld)
endif()

zephyr_include_directories(${ZEPHYR_BASE}/soc/nordic/nrf54l)
add_subdirectory(${ZEPHYR_BASE}/soc/nordic/common ${CMAKE_BINARY_DIR}/common)

zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LS05B_ENGA NRF_SKIP_TAMPC_SETUP)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LS05B_ENGA NRF54LS05B_ENGA_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LS05B_ENGA_CPUAPP NRF_APPLICATION)

zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LV10A_ENGA NRF_SKIP_TAMPC_SETUP)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LV10A_ENGA NRF54LV10A_ENGA_XXAA)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LV10A_ENGA_CPUAPP  NRF_APPLICATION)
zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF54LV10A_ENGA_CPUFLPR NRF_FLPR)
