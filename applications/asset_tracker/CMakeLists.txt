#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

include(../../cmake/boilerplate.cmake)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(asset_tracker)
zephyr_compile_definitions(PROJECT_NAME=${PROJECT_NAME})

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END
zephyr_include_directories(src)

# Include application events and configuration headers
zephyr_library_include_directories(
  src/orientation_detector
  src/ui
  src/cloud_codec
  )

# Application sources
add_subdirectory(src/orientation_detector)
add_subdirectory(src/ui)
add_subdirectory(src/cloud_codec)
add_subdirectory(src/gps_controller)
add_subdirectory(src/env_sensors)

