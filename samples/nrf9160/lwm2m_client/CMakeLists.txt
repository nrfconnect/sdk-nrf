#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lwm2m_client)
zephyr_compile_definitions(PROJECT_NAME=${PROJECT_NAME})

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
zephyr_include_directories(src)
# NORDIC SDK APP END

# Include application events and configuration headers
zephyr_library_include_directories(
	src/lwm2m
	src/ui
	src/ipso
	src/events
	src/sensors
	src/modules
	)

# Application sources
add_subdirectory(src/lwm2m)
add_subdirectory(src/ui)
add_subdirectory(src/ipso)
add_subdirectory(src/events)
add_subdirectory(src/sensors)
add_subdirectory(src/modules)
