#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if(CONF_FILE)
  if(${CONF_FILE} MATCHES "prj_(.*).conf")
    set(CONF_FILE_BUILD_TYPE ${CMAKE_MATCH_1})
    set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD}_${CONF_FILE_BUILD_TYPE}.overlay")
  endif()
  set(remote_CONF_FILE ${CONF_FILE})
endif()

set(ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(event_manager_proxy)

add_subdirectory(common_config)
add_subdirectory(common_events)
add_subdirectory(modules)

target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_sources(app PRIVATE
  src/main.c
  src/data.c
  src/simple.c
)
