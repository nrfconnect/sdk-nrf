#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(json_common_test)

# Set CMake path variables for convenience
set(ASSET_TRACKER_V2_DIR ${ZEPHYR_BASE}/../nrf/applications/asset_tracker_v2)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_include_directories(app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR} ${ASSET_TRACKER_V2_DIR}/src/cloud/cloud_codec/
	${CMAKE_CURRENT_SOURCE_DIR} ${ZEPHYR_BASE}/../nrfxlib/nrf_modem/include/)

target_sources(app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR} mock/date_time_mock.c
	${CMAKE_CURRENT_SOURCE_DIR} ${ASSET_TRACKER_V2_DIR}/src/cloud/cloud_codec/json_common.c
	${CMAKE_CURRENT_SOURCE_DIR} ${ASSET_TRACKER_V2_DIR}/src/cloud/cloud_codec/json_helpers.c)

target_compile_options(app PRIVATE
  	-DCONFIG_ASSET_TRACKER_V2_APP_VERSION_MAX_LEN=20)
