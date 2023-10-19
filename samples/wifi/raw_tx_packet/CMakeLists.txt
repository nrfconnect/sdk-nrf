#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_raw_tx_packet)

target_include_directories(app PUBLIC ${ZEPHYR_BASE}/subsys/net/ip)

target_sources(app PRIVATE
	src/wifi_connection.c
	src/main.c
)
