#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(untitled_sample_demo)

target_sources(app PRIVATE src/main.c)

if(CONFIG_MEMFAULT)
  zephyr_include_directories(memfault_config)
endif()
