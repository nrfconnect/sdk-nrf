#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.8)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

# Application sources
add_subdirectory(src/fem)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/dtm.c
  src/dtm_hw.c
)
# NORDIC SDK APP END
zephyr_library_include_directories(.)
