#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This directory contains C sources for the Wi-Fi keys driver

list(APPEND wifi_keys_driver_include_dirs
  ${CMAKE_CURRENT_LIST_DIR}/include
)

list(APPEND wifi_keys_driver_sources
  ${CMAKE_CURRENT_LIST_DIR}/src/wifi_keys.c
)
