# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Set path to the latest version of nrfx
set(NRFX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../nrfx/)
set(NRFX_DIR ${NRFX_DIR} CACHE STRING "Path to nrfx")

zephyr_get(NRFX_DIR SYSBUILD GLOBAL)

if(NOT DEFINED NRFX_DIR)
  set(NRFX_DIR ${ZEPHYR_CURRENT_MODULE_DIR}/nrfx/ CACHE PATH "nrfx Directory")
endif()

zephyr_include_directories_ifdef(CONFIG_HAS_NRFX .)
add_subdirectory(${ZEPHYR_BASE}/modules/hal_nordic ${CMAKE_CURRENT_BINARY_DIR}/modules/hal_nordic)
add_subdirectory_ifdef(CONFIG_HAS_NRFX nrfx)
