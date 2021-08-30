#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(location_test)

# Generate runner for the test
test_runner_generate(src/location_test.c)

cmock_handle(../../../include/modem/at_cmd.h)
cmock_handle(../../../include/modem/modem_key_mgmt.h)
cmock_handle(../../../include/modem/modem_info.h)
cmock_handle(../../../lib/multicell_location/http_request.h)
cmock_handle(${ZEPHYR_BASE}/../nrfxlib/nrf_modem/include/nrf_modem_gnss.h)
cmock_handle(${ZEPHYR_BASE}/../nrfxlib/nrf_modem/include/nrf_socket.h)

zephyr_include_directories(../../../../nrfxlib/nrf_modem/include)

target_sources(app PRIVATE src/location_test.c)
