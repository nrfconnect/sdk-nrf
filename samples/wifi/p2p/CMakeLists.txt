#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_p2p)

target_include_directories(app PUBLIC ${ZEPHYR_BASE}/subsys/net/ip)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_P2P_SAMPLE_CLI_MODE app PRIVATE src/p2p_cli.c)
target_sources_ifdef(CONFIG_P2P_SAMPLE_GO_MODE app PRIVATE src/p2p_go.c)
