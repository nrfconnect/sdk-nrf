#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(suit_smp_transfer)

target_sources(app PRIVATE src/main.c)

# This project uses orginal sdk-zephyr C source code
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src)
target_sources_ifdef(CONFIG_MCUMGR_TRANSPORT_BT app PRIVATE $ENV{ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/bluetooth.c)
