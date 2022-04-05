#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

if (CONF_FILE)
  if(${CONF_FILE} MATCHES "prj_(.*).conf")
    set(CONF_FILE_BUILD_TYPE ${CMAKE_MATCH_1})
  endif()

  # Use config file with the same name in cpunet.
  set(cpunet_CONF_FILE ${CONF_FILE})
  set(DTC_OVERLAY_FILE "${CONF_FILE_BUILD_TYPE}.overlay")
  set(cpunet_DTC_OVERLAY_FILE "${CONF_FILE_BUILD_TYPE}.overlay")

  # Use config overlay with the same name in cpunet.
  if(OVERLAY_CONFIG)
    set(cpunet_OVERLAY_CONFIG ${OVERLAY_CONFIG})
  endif()
endif()

if("${BOARD}" STREQUAL "nrf5340dk_nrf5340_cpuapp")
  set(CHILD_BOARD "nrf5340dk_nrf5340_cpunet")
else()
  message(FATAL_ERROR "${BOARD} is not supported for this sample")
endif()

set(ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ipc_service_host)

target_sources(app PRIVATE src/main.c)
