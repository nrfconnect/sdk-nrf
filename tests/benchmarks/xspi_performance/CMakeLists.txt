#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(xspi_performance)

target_sources_ifdef(CONFIG_CPU_LOAD app PRIVATE src/cpu_load_monitor.c)
target_sources(app PRIVATE src/main.c)
