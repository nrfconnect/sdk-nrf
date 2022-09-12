#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_nrf_cloud)
zephyr_compile_definitions(PROJECT_NAME=${PROJECT_NAME})

# Generate test runner
test_runner_generate(src/main.c)

# Declare CMock handle
cmock_handle(${ZEPHYR_BASE}/../nrf/subsys/net/lib/nrf_cloud/include/nrf_cloud_transport.h)

# NRF CLOUD TEST START
FILE(GLOB app_sources src/main.c)
target_sources(app PRIVATE ${app_sources})
target_include_directories(app
	PRIVATE
	${ZEPHYR_BASE}/../nrf/subsys/net/lib/nrf_cloud/include/
	)
# NRF CLOUD TEST END
