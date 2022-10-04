#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

project(wrap_test)

# add module test_code
set(SOURCES src/test_code/test_code.c)

# add module call
list(APPEND SOURCES src/wrap_test.c)

# add test file
list(APPEND SOURCES src/call/call.c)

find_package(Zephyr REQUIRED COMPONENTS ncs_unity unittest HINTS $ENV{ZEPHYR_BASE})

# generate runner for the test
test_runner_generate(src/wrap_test.c)

# create mocks for test_code functions
cmock_handle(src/test_code/test_code.h test_code)

target_include_directories(testbinary PRIVATE src)

target_include_directories(testbinary PRIVATE ./src/call)

target_include_directories(testbinary PRIVATE .)
target_include_directories(testbinary PRIVATE src/test_code)
