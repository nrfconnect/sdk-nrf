#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(remote)

#Include application application event headers
zephyr_library_include_directories(../src/events)

target_sources(app PRIVATE src/main.c)

# Add include directory for board specific CAF def files
zephyr_include_directories(
  ../configuration/
  ../configuration/${NORMALIZED_BOARD_TARGET}
)

add_subdirectory(../src/modules "${CMAKE_CURRENT_BINARY_DIR}/modules")
