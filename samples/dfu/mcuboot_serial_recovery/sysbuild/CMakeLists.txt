#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(BOARD STREQUAL "nrf54lm20dongle/nrf54lm20b/cpuapp")
  list(APPEND SB_EXTRA_CONF_FILE
       ${CMAKE_CURRENT_LIST_DIR}/boards/nrf54lm20dongle_nrf54lm20b_cpuapp.conf)
  set(SB_EXTRA_CONF_FILE ${SB_EXTRA_CONF_FILE} CACHE STRING
      "Board-specific sysbuild Kconfig fragments" FORCE)
endif()

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sysbuild LANGUAGES)