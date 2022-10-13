#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_mqtt_test)
set(NRF_SDK_DIR ${ZEPHYR_BASE}/../nrf)
cmake_path(NORMAL_PATH NRF_SDK_DIR)

# NRF CLOUD TEST START
FILE(GLOB app_sources src/main.c)
target_sources(app PRIVATE ${app_sources})

target_include_directories(app
	PRIVATE
	src
	${NRF_SDK_DIR}/subsys/net/lib/nrf_cloud/include
	${ZEPHYR_BASE}/subsys/testsuite/include
	${ZEPHYR_BASE}/../modules/lib/cjson
)

set_source_files_properties(
	${ZEPHYR_BASE}/subsys/net/lib/sockets/getaddrinfo.c
	DIRECTORY ${ZEPHYR_BASE}/subsys/net/lib/sockets
	PROPERTIES HEADER_FILE_ONLY ON
)
# NRF CLOUD TEST END
