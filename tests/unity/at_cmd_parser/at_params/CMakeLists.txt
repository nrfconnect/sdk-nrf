#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(test_at_params)

# create mock for the at_params_alloc header
cmock_handle(${ZEPHYR_BASE}/../nrf/lib/at_cmd_parser/include/at_params_alloc.h)

# generate runner for the test
test_runner_generate(src/at_params_test.c)

# add test file
target_sources(app PRIVATE src/at_params_test.c)
target_include_directories(app PRIVATE .)
