# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library_sources(
  ${ZEPHYR_BASE}/soc/nordic/nrf54l/soc.c
  ${ZEPHYR_BASE}/soc/nordic/validate_rram_partitions.c
  )

zephyr_include_directories(${ZEPHYR_BASE}/soc/nordic/nrf54l)
add_subdirectory(${ZEPHYR_BASE}/soc/nordic/common ${CMAKE_BINARY_DIR}/common)
