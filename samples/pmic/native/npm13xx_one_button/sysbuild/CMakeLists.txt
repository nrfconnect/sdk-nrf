# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(SHIELD STREQUAL "npm1300_ek")
  set(EXTRA_DTC_OVERLAY_FILE npm1300.overlay CACHE INTERNAL "extra overlay")
elseif(SHIELD STREQUAL "npm1304_ek")
  set(EXTRA_DTC_OVERLAY_FILE npm1304.overlay CACHE INTERNAL "extra overlay")
else()
  message(FATAL_ERROR "either npm1300_ek or npm1304_ek must be selected with --shield")
endif()

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sysbuild LANGUAGES)
