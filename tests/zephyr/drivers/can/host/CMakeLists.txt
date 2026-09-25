# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Extends upstream CAN host test suite

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(can_host_extended)

set(src_dir ${ZEPHYR_BASE}/tests/drivers/can/host/src)

file(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
