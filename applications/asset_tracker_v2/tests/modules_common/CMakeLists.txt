#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(modules_common)

# Convenience CMake variable
set(ASSET_TRACKER_V2_DIR ../..)

# Generate runner for the test
test_runner_generate(src/modules_common_test.c)

# Add Unit Under Test source files
target_sources(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/modules/modules_common.c)

# Add test source file
target_sources(app PRIVATE src/modules_common_test.c)

# Add directories
target_include_directories(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/modules)
