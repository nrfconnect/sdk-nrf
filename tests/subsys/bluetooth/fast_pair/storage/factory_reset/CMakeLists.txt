#
# Copyright (c) 2023-2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Fast Pair storage factory reset unit test")

# Add test sources
target_sources(app PRIVATE
  src/main.c
  ../common/src/common_utils.c
)
target_include_directories(app PRIVATE ../common/include)

# Add Fast Pair storage as part of the test
set(NCS_FAST_PAIR_BASE ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair)
add_subdirectory(${NCS_FAST_PAIR_BASE}/fp_storage fp_storage)
target_link_libraries(app PRIVATE fp_storage)

# For strnlen()
target_compile_definitions(app PRIVATE _POSIX_C_SOURCE=200809L)
