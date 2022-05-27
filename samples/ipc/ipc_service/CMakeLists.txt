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

  # Use config file with the same name in remote.
  set(remote_CONF_FILE ${CONF_FILE})
  set(DTC_OVERLAY_FILE "${CONF_FILE_BUILD_TYPE}.overlay")
  set(remote_DTC_OVERLAY_FILE "${CONF_FILE_BUILD_TYPE}.overlay")
endif()

# Use config overlay with the same name in remote.
if(OVERLAY_CONFIG)
  set(remote_OVERLAY_CONFIG ${OVERLAY_CONFIG})
endif()

set(ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ipc_service_host)

target_sources(app PRIVATE src/main.c)
