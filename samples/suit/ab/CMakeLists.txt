#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(suit_ab)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/suit_manifests_state.c)
target_sources(app PRIVATE src/suit_components_state.c)
target_sources(app PRIVATE src/device_management.c)
zephyr_library_include_directories(src)

zephyr_library_link_libraries(suit_manifest_variables)
zephyr_library_link_libraries(suit_ipuc)

# This project uses orginal sdk-zephyr C source code
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src)
target_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_BT app PRIVATE $ENV{ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/bluetooth.c)
