#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(memfault_sample)

# NORDIC SDK APP START
add_subdirectory(src)
# NORDIC SDK APP END

zephyr_include_directories(src)
zephyr_include_directories(config)
