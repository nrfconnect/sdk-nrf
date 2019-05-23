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
  src/battery_monitor
  )

# Application sources
add_subdirectory(src/orientation_detector)
add_subdirectory(src/ui)
add_subdirectory(src/cloud_codec)
add_subdirectory(src/gps_controller)
if (CONFIG_BATTERY_MONITOR)
add_subdirectory(src/battery_monitor)
endif()

include_directories(src/env_sensors)

if (CONFIG_USE_BME680_BSEC)
	target_sources(app PRIVATE src/env_sensors//bsec.c)

	set (bsec_dir ${CMAKE_CURRENT_SOURCE_DIR}/src/env_sensors/BSEC_1.4.7.3_Generic_Release)
	include_directories(${bsec_dir}/API)
	target_sources(app PRIVATE ${bsec_dir}/examples/bsec_integration.c)
	include_directories(${bsec_dir}/examples)
	target_sources(app PRIVATE ${bsec_dir}/API/bme680.c)

	set(BSEC_LIB_DIR ${bsec_dir}/algo/bin/Normal_version/gcc/Cortex_M4F)
	set(BSEC_INCLUDE_DIR ${bsec_dir}/algo/bin/Normal_version/gcc/Cortex_M4F)
	add_library(bsec_lib STATIC IMPORTED GLOBAL)
	add_dependencies(bsec_lib math_lib bsec_target)
	set_target_properties(bsec_lib PROPERTIES IMPORTED_LOCATION "${BSEC_LIB_DIR}/libalgobsec.a")
	set_target_properties(bsec_lib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${BSEC_INCLUDE_DIR})
	target_link_libraries(bsec_lib INTERFACE -L${LIBC_LIBRARY_DIR})
	target_link_libraries(app PUBLIC bsec_lib)
else()
	target_sources(app PRIVATE src/env_sensors/env_sensors.c)
endif()
