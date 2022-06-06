#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Fast Pair storage unit test")

# Add test sources
target_sources(app PRIVATE
	       src/main.c
	       src/settings_mock.c
)
target_include_directories(app PRIVATE include)

# Add needed Fast Pair storage as part of the test (app target)
get_filename_component(NRF_BASE ${CMAKE_CURRENT_LIST_DIR}/../../../../../../ DIRECTORY)
set(NCS_FAST_PAIR_BASE ${NRF_BASE}/subsys/bluetooth/services/fast_pair)
target_sources(app PRIVATE ${NCS_FAST_PAIR_BASE}/fp_storage.c)
target_include_directories(app PRIVATE ${NCS_FAST_PAIR_BASE}/include)
