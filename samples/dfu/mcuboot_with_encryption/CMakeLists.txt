#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(smp_svr)

# Import all sources for SMP Server, as this is the application
# that is being used for demonstrating MCUboot with image encryption
# and DFU of encrypted images.
target_sources(app PRIVATE ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/main.c)
target_sources(app PRIVATE ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/bluetooth.c)
