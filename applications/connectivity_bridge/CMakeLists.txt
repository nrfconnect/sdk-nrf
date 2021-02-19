#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END

# Include application events and disk files
zephyr_library_include_directories(
  src/events
  )

# Application sources
add_subdirectory(src/disk)
add_subdirectory(src/events)
add_subdirectory(src/modules)
