#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(NONE)

FILE(GLOB app_sources ${CMAKE_CURRENT_LIST_DIR}/src/*.c)
target_sources(app PRIVATE ${app_sources})
