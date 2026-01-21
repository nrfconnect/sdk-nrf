# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library_sources(
  soc.c
)
zephyr_library_sources_ifdef(CONFIG_SOC_NRF7120_PREENG
  soc_preeng.c
)
zephyr_include_directories(.)
add_subdirectory(${ZEPHYR_BASE}/soc/nordic/common ${CMAKE_BINARY_DIR}/common)

# Ensure that image size aligns with 16 bytes so that MRAMC finalizes all writes
# for the image correctly
zephyr_linker_sources(SECTIONS SORT_KEY zzz_place_align_at_end align.ld)

# We need a buffer in memory in a static location which can be used by
# the KMU peripheral. The KMU has a static destination address,for nRF7120
# this address is defined as 0x200F_FF00, which is the last 256 Bytes in the SRAM.
if(NOT CONFIG_BUILD_WITH_TFM AND CONFIG_PSA_NEED_CRACEN_KMU_DRIVER)
  # Exclamation mark is printable character with the lowest number in ASCII table.
  # We are sure that this file will be included first.
  zephyr_linker_sources(RAM_SECTIONS SORT_KEY ! kmu_push_area_section.ld)
endif()
