# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

add_subdirectory(${ZEPHYR_BASE}/soc/nordic/common ${CMAKE_CURRENT_BINARY_DIR}/soc_nordic_common)

if(CONFIG_SOC_NRF9251_CPUAPP)
  zephyr_library_sources(soc.c)
  zephyr_library_sources(${ZEPHYR_BASE}/soc/nordic/nrf92/soc_power.c)
  zephyr_code_relocate(
    FILES ${ZEPHYR_BASE}/soc/nordic/nrf92/soc_power.c
    FILTER ".*\\.cache_retain_and_sleep"
    LOCATION PMLocalRamfunc_TEXT
  )
  zephyr_library_sources_ifdef(CONFIG_PM_S2RAM
    ${ZEPHYR_BASE}/soc/nordic/nrf92/soc_pm_s2ram.c
  )
endif()

zephyr_include_directories(
  .
  ${ZEPHYR_BASE}/soc/nordic/common
  ${ZEPHYR_BASE}/soc/nordic/nrf92
)

# Ensure that image size aligns with 16 bytes so that MRAMC finalizes all writes
# for the image correctly
zephyr_linker_sources(SECTIONS SORT_KEY zzz_place_align_at_end ${ZEPHYR_BASE}/soc/nordic/nrf92/align.ld)
