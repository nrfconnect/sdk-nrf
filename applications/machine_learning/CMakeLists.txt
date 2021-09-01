#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)


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

set(hci_rpmsg_CONF_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/hci_rpmsg_${CMAKE_BUILD_TYPE}.conf
  )

# Use static partition layout to ensure consistency between builds.
# This is to ensure settings storage will be at the same location after the DFU.
set(PM_STATIC_YML_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static_${CMAKE_BUILD_TYPE}.yml
  )

# Define configuration files.
set(CONF_FILE "configuration/${BOARD}/app_${CMAKE_BUILD_TYPE}.conf")

################################################################################

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Machine learning"
        VERSION 0.1)

################################################################################

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  )
# NORDIC SDK APP END

# Include application events and configuration headers
target_include_directories(app PRIVATE
  src/events
  src/util
)

zephyr_include_directories(
  configuration/common
  configuration/${BOARD}
)

# Application sources
add_subdirectory(src/events)
add_subdirectory(src/modules)
add_subdirectory(src/util)

if(CONFIG_BT_RPMSG_NRF53)
  assert_exists(hci_rpmsg_CONF_FILE)
endif()
