#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

FILE(GLOB app_sources src/*.c)
# NORDIC SDK APP START
target_sources(app PRIVATE ${app_sources})
# NORDIC SDK APP END
