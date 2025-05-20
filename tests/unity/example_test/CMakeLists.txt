#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(example_test)

# generate runner for the test
test_runner_generate(src/example_test.c)

# create mock for foo module. Add header to relative path 'foo'. That is to
# allow including header using <foo/foo.h>
cmock_handle(src/foo/foo.h foo)

# add module foo
target_sources(app PRIVATE src/foo/foo.c)
target_include_directories(app PRIVATE src)

# add module uut
target_sources(app PRIVATE src/uut/uut.c)
target_include_directories(app PRIVATE ./src/uut)

# add test file
target_sources(app PRIVATE src/example_test.c)
target_include_directories(app PRIVATE .)
target_include_directories(app PRIVATE src/foo)

# Preinclude file to the module under test to redefine IS_ENABLED() macro
# which is used in the module.
set_property(SOURCE src/uut/uut.c PROPERTY COMPILE_FLAGS
	"-include src/example_test.h")
