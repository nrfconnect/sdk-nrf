#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(location_test)

# Generate runner for the test
test_runner_generate(src/location_test.c)

cmock_handle(../../../include/modem/modem_key_mgmt.h)
cmock_handle(../../../include/net/rest_client.h)
cmock_handle(${ZEPHYR_BASE}/../nrfxlib/nrf_modem/include/nrf_modem_gnss.h)
cmock_handle(${ZEPHYR_BASE}/../nrfxlib/nrf_modem/include/nrf_modem_at.h)

zephyr_include_directories(${ZEPHYR_BASE}/../nrfxlib/nrf_modem/include)

target_sources(app PRIVATE src/location_test.c)
