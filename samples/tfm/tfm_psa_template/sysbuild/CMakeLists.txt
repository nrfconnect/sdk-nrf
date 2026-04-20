#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# nRF54L needs extra sysbuild settings (Pure signature mode, KMU keyfile
# generation, sample PSA cert reference); nRF5340 and nRF91x get the right
# defaults from Kconfig.
if("${BOARD}" MATCHES "^nrf54l")
  list(APPEND SB_EXTRA_CONF_FILE
    "${CMAKE_CURRENT_LIST_DIR}/nrf54l.conf")
endif()

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sysbuild LANGUAGES)
