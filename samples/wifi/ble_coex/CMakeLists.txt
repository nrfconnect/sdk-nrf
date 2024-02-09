#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(DTC_OVERLAY_FILE "dts.overlay")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_sta)

target_sources(app PRIVATE
	src/bt_throughput_test.c
	src/main.c
)

# For net_private.h
target_include_directories(app PUBLIC
	${ZEPHYR_BASE}/subsys/net/ip
)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)
