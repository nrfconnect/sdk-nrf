#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Fast Pair storage of Account Keys unit test")

# Add test sources
target_sources(app PRIVATE
	       src/main.c
	       src/settings_mock.c
	       src/test_corrupted_data.c
	       ../common/src/common_utils.c
)
target_include_directories(app PRIVATE include)
target_include_directories(app PRIVATE ../common/include)

# Add Fast Pair storage as part of the test
set(NCS_FAST_PAIR_BASE ${NRF_DIR}/subsys/bluetooth/services/fast_pair)
add_subdirectory(${NCS_FAST_PAIR_BASE}/fp_storage fp_storage)
target_link_libraries(app PRIVATE fp_storage)
