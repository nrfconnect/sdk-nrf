#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_fuel_gauge_test)

# generate runner for the test
test_runner_generate(src/nrf_fuel_gauge_test.c)

target_include_directories(app PRIVATE
	src
	data
)

# add test file
target_sources(app PRIVATE src/nrf_fuel_gauge_test.c)
