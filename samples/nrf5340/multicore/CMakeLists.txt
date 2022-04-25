#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if("${BOARD}" STREQUAL "nrf5340dk_nrf5340_cpuapp")
  set(CHILD_BOARD "nrf5340dk_nrf5340_cpunet")
else()
  message(FATAL_ERROR "${BOARD} is not supported for this sample")
endif()

set(ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

target_sources(app PRIVATE src/main.c)
