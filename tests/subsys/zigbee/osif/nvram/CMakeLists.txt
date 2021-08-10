#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# Stub for ZBOSS callout
add_compile_definitions(zb_nvram_erase_finished=zb_nvram_erase_finished_stub)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(zigbee_osif_nvram_test)

target_sources(app PRIVATE
  src/main.c
)
