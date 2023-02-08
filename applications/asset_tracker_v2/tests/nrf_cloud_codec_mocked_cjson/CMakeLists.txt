#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_mocked_cjson_test)

set(ASSET_TRACKER_V2_DIR ../..)

# generate runner for the test
test_runner_generate(src/nrf_cloud_codec_mocked_test.c)

# create mock
cmock_handle(${ASSET_TRACKER_V2_DIR}/src/cloud/cloud_codec/json_helpers.h)
cmock_handle(${NRF_DIR}/include/date_time.h)
cmock_handle(${NRF_DIR}/include/net/nrf_cloud.h)
cmock_handle(src/cJSON_to_mock.h)

# add test file
target_sources(app PRIVATE src/nrf_cloud_codec_mocked_test.c)
target_include_directories(app PRIVATE src/)

target_include_directories(app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR} ${ASSET_TRACKER_V2_DIR}/src/cloud/cloud_codec/
	${CMAKE_CURRENT_SOURCE_DIR} ${NRF_DIR}/../nrfxlib/nrf_modem/include/)

target_include_directories(app PRIVATE ${NRF_DIR}/../modules/lib/cjson/)
target_include_directories(app PRIVATE ${NRF_DIR}/modules/cjson/include/)

# Add cloud codec module (unit under test)
target_sources(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/cloud/cloud_codec/nrf_cloud/nrf_cloud_codec.c)
target_sources(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/cloud/cloud_codec/cloud_codec_ringbuffer.c)

target_compile_options(app PRIVATE
	-DCONFIG_ASSET_TRACKER_V2_APP_VERSION_MAX_LEN=20
	-DCONFIG_LTE_NEIGHBOR_CELLS_MAX=10
)
