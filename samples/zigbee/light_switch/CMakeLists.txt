#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project("Light Switch")

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
)
target_include_directories(app PRIVATE include)
# NORDIC SDK APP END
