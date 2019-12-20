#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("Event Manager unit tests")

# Include event headers
zephyr_library_include_directories(src/events)

# Add test sources
target_sources(app PRIVATE src/main.c)
add_subdirectory(src/events)
add_subdirectory(src/modules)
