#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(qos_test)

# generate runner for the test
test_runner_generate(src/qos_test.c)

target_include_directories(app PRIVATE src)

# add test file
target_sources(app PRIVATE src/qos_test.c)
