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

# Add needed Fast Pair crypto as part of the test (app target)
get_filename_component(NRF_BASE ${CMAKE_CURRENT_LIST_DIR}/../../../../../../ DIRECTORY)
set(NCS_FAST_PAIR_BASE ${NRF_BASE}/subsys/bluetooth/services/fast_pair)
target_sources(app PRIVATE ${NCS_FAST_PAIR_BASE}/fp_crypto_common.c)
target_sources_ifdef(CONFIG_BT_FAST_PAIR_CRYPTO_MBEDTLS app
		     PRIVATE ${NCS_FAST_PAIR_BASE}/fp_crypto_mbedtls.c)
target_sources_ifdef(CONFIG_BT_FAST_PAIR_CRYPTO_TINYCRYPT app
		     PRIVATE ${NCS_FAST_PAIR_BASE}/fp_crypto_tinycrypt.c)
target_include_directories(app PRIVATE ${NCS_FAST_PAIR_BASE}/include)
