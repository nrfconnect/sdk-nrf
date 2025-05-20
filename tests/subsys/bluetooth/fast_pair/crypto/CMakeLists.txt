#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Fast Pair crypto unit test")

# Add test sources
target_sources(app PRIVATE src/main.c)

# Add Fast Pair crypto as part of the test
set(NCS_FAST_PAIR_BASE ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/services/fast_pair)
add_subdirectory(${NCS_FAST_PAIR_BASE}/fp_crypto fp_crypto)
target_link_libraries(app PRIVATE fp_crypto)
