#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
cmake_minimum_required(VERSION 3.8.2)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

target_sources(app PRIVATE
	src/main.c
	src/model_handler.c)
target_include_directories(app PRIVATE include)
