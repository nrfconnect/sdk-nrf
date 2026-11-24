#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if(CONFIG_BC_MARKER)
  zephyr_library()
  zephyr_library_sources(src/bc_marker.c)
endif()
