#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(azure-fota)

# NORDIC SDK APP START
target_sources(app PRIVATE
	src/main.c
	)

target_include_directories(app PRIVATE src)
# NORDIC SDK APP END
