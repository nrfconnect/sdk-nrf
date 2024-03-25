#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

set(ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR})
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Sensor manager sample")

target_sources(app PRIVATE src/main.c)

# Add include directory for board specific CAF def files
zephyr_include_directories(
  configuration/
  configuration/${NORMALIZED_BOARD_TARGET}
)

add_subdirectory(src/modules)
