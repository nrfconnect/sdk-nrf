#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wrap_test)

# generate runner for the test
test_runner_generate(src/wrap_test.c)

# create mocks for test_code functions
cmock_handle(src/test_code/test_code.h test_code
  WORD_EXCLUDE "IGNORE_ME"
)

# add module test_code
target_sources(app PRIVATE src/test_code/test_code.c)
target_include_directories(app PRIVATE src)

# add module call
target_sources(app PRIVATE src/call/call.c)
target_include_directories(app PRIVATE ./src/call)

# add test file
target_sources(app PRIVATE src/wrap_test.c)
target_include_directories(app PRIVATE .)
target_include_directories(app PRIVATE src/test_code)
