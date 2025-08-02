#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(APPLICATION_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bootloader)

zephyr_library_sources(../src/main.c)

# Dummy config values due to building a proper project
zephyr_compile_definitions(CONFIG_PARTITION_MANAGER_ENABLED=y)
zephyr_compile_definitions(PM_S0_ADDRESS=0x10000)
zephyr_compile_definitions(PM_S1_ADDRESS=0x80000)
