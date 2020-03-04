#
# Copyright (c) 2018 - 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE ZDebug)
endif()

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}")
  message(FATAL_ERROR
          "Board ${BOARD} is not supported.\n"
          "Please make sure board specific configuration files are added to "
          "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}")
endif()

if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/app_${CMAKE_BUILD_TYPE}.conf")
  message(FATAL_ERROR
          "Configuration file for build type ${CMAKE_BUILD_TYPE} is missing.\n"
          "Please add file ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/app_${CMAKE_BUILD_TYPE}.conf")
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/dts.overlay")
  set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/dts.overlay")
endif()

set(mcuboot_CONF_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/mcuboot_${CMAKE_BUILD_TYPE}.conf
  )

# Use static partition layout to ensure consistency between builds.
# This is to ensure settings storage will be at the same location after the DFU.
set(PM_STATIC_YML_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static_${CMAKE_BUILD_TYPE}.yml
  )

# Define configuration files.
set(CONF_FILE "configuration/${BOARD}/app_${CMAKE_BUILD_TYPE}.conf")

################################################################################

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("nRF52 Desktop"
        VERSION 0.1)

################################################################################

assert(CONFIG_DESKTOP_HID_REPORT_DESC "HID report descriptor file must be specified")

target_sources(app PRIVATE
  src/main.c
  ${CONFIG_DESKTOP_HID_REPORT_DESC}
  )

# Include application events and configuration headers
zephyr_library_include_directories(
  src/events
  src/util
  configuration/common
  configuration/${BOARD}
  )

# Application sources
add_subdirectory(src/events)
add_subdirectory(src/hw_interface)
add_subdirectory(src/modules)
add_subdirectory(src/util)

if(CONFIG_BOOTLOADER_MCUBOOT)
  assert_exists(mcuboot_CONF_FILE)
endif()
