#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.3)

set(NRF_CLOUD "nrf-cloud")
set(AWS_IOT "aws")
set(AZURE_IOT_HUB "azure")
set(AGPS "agps")
set(PGPS "pgps")
set(DEBUG "debug")
set(LOW_POWER "low-power")

if(NOT DEFINED APP_CONFIG_LIST)
  message(WARNING "Configuration list is empty, using nRF Cloud as default cloud provider")
  list(APPEND OVERLAY_CONFIG "configuration/nrf-cloud.conf")
elseif(NRF_CLOUD IN_LIST APP_CONFIG_LIST)
  list(APPEND OVERLAY_CONFIG "configuration/nrf-cloud.conf")
elseif(AWS_IOT IN_LIST APP_CONFIG_LIST)
  list(APPEND OVERLAY_CONFIG "configuration/aws.conf")
elseif(AZURE_IOT_HUB IN_LIST APP_CONFIG_LIST)
  list(APPEND OVERLAY_CONFIG "configuration/azure.conf")
else()
  message(FATAL_ERROR "No cloud provider found in the APP_CONFIG_LIST")
endif()

if(AGPS IN_LIST APP_CONFIG_LIST)
  list(APPEND OVERLAY_CONFIG "configuration/agps.conf")
endif()

if(PGPS IN_LIST APP_CONFIG_LIST)
  list(APPEND OVERLAY_CONFIG "configuration/pgps.conf")
endif()

if(DEBUG IN_LIST APP_CONFIG_LIST AND LOW_POWER IN_LIST APP_CONFIG_LIST)
  message(FATAL_ERROR "Debug and low-power configuration cannot be set at the same time")
elseif(DEBUG IN_LIST APP_CONFIG_LIST)
  list(APPEND OVERLAY_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/configuration/debug.conf")
elseif(LOW_POWER IN_LIST APP_CONFIG_LIST)
  list(APPEND OVERLAY_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/configuration/low-power.conf")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(asset_tracker_v2)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END

# Include source directory
target_include_directories(app PRIVATE src)
target_include_directories(app PRIVATE boards/${BOARD}/)

# Application directories
add_subdirectory(src/modules)
add_subdirectory(src/events)

add_subdirectory_ifdef(CONFIG_CLOUD_MODULE src/cloud)
add_subdirectory_ifdef(CONFIG_SENSOR_MODULE src/ext_sensors)
add_subdirectory_ifdef(CONFIG_WATCHDOG_APPLICATION src/watchdog)
