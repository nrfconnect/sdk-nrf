#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(tnep_test)

# generate runner for the test
test_runner_generate(src/tnep_tests.c)

# add test file
target_sources(app PRIVATE src/tnep_tests.c)
target_include_directories(app PRIVATE .)
