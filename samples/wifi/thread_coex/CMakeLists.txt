#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(DTC_OVERLAY_FILE "dts.overlay")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_thread_coex)

target_sources(app PRIVATE
	src/ot_utils.c
	src/zperf_utils.c
	src/ot_coex_functions.c
	src/main.c
)

# For net_private.h
target_include_directories(app PUBLIC
	${ZEPHYR_BASE}/subsys/net/ip
)
