#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dect_rpc)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_SAMPLE_DECT_RPC_CLIENT_IPERF3 app PRIVATE src/iperf3_shell.c)
