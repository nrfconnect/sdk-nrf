#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(NONE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FILE(GLOB app_src src/*.c)

target_sources(app PRIVATE ${app_src})
target_include_directories(app PRIVATE src)

add_subdirectory(test_cases)
