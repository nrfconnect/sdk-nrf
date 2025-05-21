cmake_minimum_required(VERSION 3.20.0)
# Top-level CMakeLists.txt for the skeleton application.
#
# Copyright (c) 2025 Open Source Foundries Limited
#
# SPDX-License-Identifier: Apache-2.0

# Standard Zephyr application boilerplate.
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(smp_svr)

target_sources(app PRIVATE ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/main.c)
target_sources(app PRIVATE ${ZEPHYR_BASE}/samples/subsys/mgmt/mcumgr/smp_svr/src/bluetooth.c)
