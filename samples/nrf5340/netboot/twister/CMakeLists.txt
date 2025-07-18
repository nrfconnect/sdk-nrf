#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(APPLICATION_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(network_core_bootloader)

zephyr_library_sources(../src/main.c)
zephyr_compile_definitions(CONFIG_PARTITION_MANAGER_ENABLED=y)
