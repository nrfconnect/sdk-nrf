#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(channel_map_updates)

target_include_directories(app PRIVATE include)
target_sources(app PRIVATE include
    src/main.c
    src/filter_algo.c
    )
# NORDIC SDK APP END