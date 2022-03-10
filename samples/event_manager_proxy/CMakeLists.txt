#
# Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/event_manager_proxy_remote/src/event_manager_proxy_remote-build/zephyr)

if("${BOARD}" STREQUAL "nrf5340dk_nrf5340_cpuapp")
  set(BOARD_REMOTE "nrf5340dk_nrf5340_cpunet")
else()
  message(FATAL_ERROR "${BOARD} is not supported for this sample")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(event_manager_proxy)

add_subdirectory(common_events)
add_subdirectory(modules)

target_sources(app PRIVATE src/main.c)

include(ExternalProject)

ExternalProject_Add(
  event_manager_proxy_remote
  SOURCE_DIR ${APPLICATION_SOURCE_DIR}/remote
  INSTALL_COMMAND ""      # This particular build system has no install command
  CMAKE_CACHE_ARGS -DBOARD:STRING=${BOARD_REMOTE}
  BUILD_BYPRODUCTS "${REMOTE_ZEPHYR_DIR}/${KERNEL_BIN_NAME}"
  # NB: Do we need to pass on more CMake variables?
  BUILD_ALWAYS True
)
