#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wifi_ble_provisioning)

target_sources(app PRIVATE src/main.c)

zephyr_library_include_directories($<TARGET_PROPERTY:wifi_prov_core,BINARY_DIR>)

target_link_libraries(app PRIVATE wifi_prov_ble)
