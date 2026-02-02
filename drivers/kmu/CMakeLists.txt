# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

dt_nodelabel(kmu_push_area_node NODELABEL nrf_kmu_reserved_push_area)

# We need a buffer in memory in a static location which can be used by
# the KMU peripheral. The KMU has a static destination address,for nRF7120
# this address is defined as 0x20000000, which is the first address in the SRAM.
if(NOT CONFIG_BUILD_WITH_TFM AND CONFIG_PSA_NEED_CRACEN_KMU_DRIVER AND NOT kmu_push_area_node)
  # Exclamation mark is printable character with the lowest number in ASCII table.
  # We are sure that this file will be included first.
  zephyr_linker_sources(RAM_SECTIONS SORT_KEY ! kmu_push_area_section.ld)
  zephyr_linker_section(NAME ".nrf_kmu_reserved_push_area" ADDRESS "${RAM_ADDR}" GROUP RAM_REGION NOINIT)
endif()
