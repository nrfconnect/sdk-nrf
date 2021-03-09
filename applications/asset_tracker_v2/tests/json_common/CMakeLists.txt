#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(json_common_test)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_include_directories(app PRIVATE
  	${CMAKE_CURRENT_SOURCE_DIR} ../../src/cloud/cloud_codec/)

target_sources(app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR} mock/date_time_mock.c
	${CMAKE_CURRENT_SOURCE_DIR} ../../src/cloud/cloud_codec/json_common.c
	${CMAKE_CURRENT_SOURCE_DIR} ../../src/cloud/cloud_codec/json_helpers.c)

target_compile_options(app PRIVATE
  	-DCONFIG_CLOUD_CODEC_LOG_LEVEL=0
  	-DCONFIG_ASSET_TRACKER_V2_APP_VERSION_MAX_LEN=20)
