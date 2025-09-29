#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_client_id)
set(NRFXLIB_MODEM_DIR ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem)

FILE(GLOB app_sources src/main.c)

target_sources(app PRIVATE ${app_sources})

target_sources(app
	PRIVATE
	${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/src/nrf_cloud_client_id.c
)

target_include_directories(app
	PRIVATE
	src
	${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/common/include
	${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/mqtt/include
	${ZEPHYR_BASE}/subsys/testsuite/include
)

if (CONFIG_NRF_MODEM_LIB)

set_source_files_properties(
	${ZEPHYR_NRF_MODULE_DIR}/lib/nrf_modem_lib/nrf_modem_lib.c
	PROPERTIES HEADER_FILE_ONLY ON
)

set_source_files_properties(
	${NRFXLIB_MODEM_DIR}/include/nrf_modem_at.h
	PROPERTIES HEADER_FILE_ONLY ON
)

set_source_files_properties(
	${ZEPHYR_NRF_MODULE_DIR}/lib/modem_jwt/modem_jwt.c
	PROPERTIES HEADER_FILE_ONLY ON
)

set_source_files_properties(
	${ZEPHYR_NRF_MODULE_DIR}/lib/hw_id/hw_id.c
	PROPERTIES HEADER_FILE_ONLY ON
)


endif()
