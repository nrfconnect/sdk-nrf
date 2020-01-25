#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

set(spm_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/spm.conf)

include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(asset_tracker)
zephyr_compile_definitions(PROJECT_NAME=${PROJECT_NAME})
zephyr_compile_definitions(_POSIX_C_SOURCE=200809L)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END
zephyr_include_directories(src)

# Include application events and configuration headers
zephyr_library_include_directories(
  src/orientation_detector
  src/ui
  src/cloud_codec
  src/env_sensors
  src/light_sensor
  src/watchdog
  )

# Application sources
add_subdirectory(src/motion)
add_subdirectory(src/ui)
add_subdirectory(src/cloud_codec)
add_subdirectory(src/gps_controller)
add_subdirectory(src/env_sensors)
add_subdirectory_ifdef(CONFIG_WATCHDOG src/watchdog)
add_subdirectory_ifdef(CONFIG_LIGHT_SENSOR src/light_sensor)

if (CONFIG_USE_BME680_BSEC)
  target_link_libraries(app PUBLIC bsec_lib)
endif()
