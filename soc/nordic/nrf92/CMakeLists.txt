# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

add_subdirectory(${ZEPHYR_BASE}/soc/nordic/common ${CMAKE_CURRENT_BINARY_DIR}/soc_nordic_common)

if(CONFIG_SOC_NRF9220_CPUAPP)
  zephyr_library_sources(soc.c)
endif()

zephyr_include_directories(. ${ZEPHYR_BASE}/soc/nordic/common)

if(CONFIG_SOC_NRF9220)
    zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF9220 NRF9220_XXAA)
    zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF9220_CPUAPP NRF_APPLICATION)
    zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF9220_CPUPPR NRF_PPR)
    zephyr_compile_definitions_ifdef(CONFIG_SOC_NRF9220_CPUFLPR NRF_FLPR)

    zephyr_linker_sources(SECTIONS SORT_KEY zzz_place_align_at_end ${ZEPHYR_BASE}/soc/nordic/nrf92/align.ld)
endif()
