#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

set(PM_STATIC_YML_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static.yml
  )

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(asset_tracker)
zephyr_compile_definitions(PROJECT_NAME=${PROJECT_NAME})
zephyr_compile_definitions(_POSIX_C_SOURCE=200809L)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END
zephyr_include_directories(src)

# Include application events and configuration headers
zephyr_library_include_directories(
  src/motion
  src/orientation_detector
  src/ui
  src/cloud_codec
  src/env_sensors
  src/light_sensor
  src/watchdog
  )

# Application sources
add_subdirectory_ifdef(CONFIG_MOTION src/motion)
add_subdirectory(src/ui)
add_subdirectory(src/cloud_codec)
add_subdirectory(src/gps_controller)
add_subdirectory_ifdef(CONFIG_ENVIRONMENT_SENSORS src/env_sensors)
add_subdirectory_ifdef(CONFIG_WATCHDOG src/watchdog)
add_subdirectory_ifdef(CONFIG_LIGHT_SENSOR src/light_sensor)

if (CONFIG_USE_BME680_BSEC)
  target_link_libraries(app PUBLIC bsec_lib)
endif()
