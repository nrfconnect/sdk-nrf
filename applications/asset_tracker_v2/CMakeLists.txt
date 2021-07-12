#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD}/${BOARD}.overlay")
  set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/boards/${BOARD}/${BOARD}.overlay")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(asset_tracker_v2)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END

# Include source directory
target_include_directories(app PRIVATE src)
target_include_directories(app PRIVATE boards/${BOARD}/)
target_include_directories(app PRIVATE ./)

# Application directories
add_subdirectory(src/modules)
add_subdirectory(src/events)

add_subdirectory_ifdef(CONFIG_CLOUD_MODULE src/cloud)
add_subdirectory_ifdef(CONFIG_SENSOR_MODULE src/ext_sensors)
add_subdirectory_ifdef(CONFIG_WATCHDOG_APPLICATION src/watchdog)
