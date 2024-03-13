#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(iso_time_sync)

target_include_directories(app PRIVATE include)
target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/combined_bis_cis.c)
target_sources(app PRIVATE src/iso_tx.c)
target_sources(app PRIVATE src/iso_rx.c)
