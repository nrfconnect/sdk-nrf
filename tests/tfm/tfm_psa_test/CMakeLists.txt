#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

if(${CMAKE_MAJOR_VERSION} EQUAL "3" AND ${CMAKE_MINOR_VERSION} EQUAL "25")
message(FATAL_ERROR "CMake versions 3.25.x are not compatible wth the PSA architecture tests.\n"
                    "Use an earlier (<= 3.24.x) or later (>= 3.26.x) version of CMake.")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tfm_psa_test)

target_sources(app PRIVATE src/main.c)

target_include_directories(app PRIVATE
  $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/install/interface/include
)
