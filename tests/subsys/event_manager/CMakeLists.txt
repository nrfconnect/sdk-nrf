#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Event Manager unit tests")

# Include event headers
zephyr_library_include_directories(src/events)

# Add test sources
target_sources(app PRIVATE src/main.c)
add_subdirectory(src/events)
add_subdirectory(src/modules)
