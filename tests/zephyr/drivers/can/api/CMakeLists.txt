# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Extends upstream CAN API test suite

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_scan_driver_test)

set(src_dir ${ZEPHYR_BASE}/tests/drivers/can/api/src)

target_sources(app PRIVATE ${src_dir}/common.c)
target_sources(app PRIVATE ${src_dir}/classic.c)
target_sources(app PRIVATE ${src_dir}/transceiver.c)
target_sources(app PRIVATE ${src_dir}/utilities.c)
target_sources_ifdef(CONFIG_CAN_FD_MODE app PRIVATE ${src_dir}/canfd.c)
target_sources_ifdef(CONFIG_CAN_STATS app PRIVATE ${src_dir}/stats.c)
