#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_codec_test)

set(ASSET_TRACKER_V2_DIR ../..)

# generate runner for the test
test_runner_generate(src/nrf_cloud_codec_test.c)

# add test file
target_sources(app PRIVATE src/nrf_cloud_codec_test.c)
target_include_directories(app PRIVATE src/)

target_include_directories(app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
	${ASSET_TRACKER_V2_DIR}/src/cloud/cloud_codec/
	${NRF_DIR}/../nrfxlib/nrf_modem/include/
	${NRF_DIR}/subsys/net/lib/nrf_cloud/include/
)

# Add cloud codec module (unit under test)
target_sources(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/cloud/cloud_codec/nrf_cloud/nrf_cloud_codec.c)
target_sources(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/cloud/cloud_codec/cloud_codec_ringbuffer.c)
target_sources(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/cloud/cloud_codec/json_helpers.c)
target_sources(app PRIVATE ${NRF_DIR}/subsys/net/lib/nrf_cloud/src/nrf_cloud_codec_internal.c)

# Mocks
target_sources(app PRIVATE ${ASSET_TRACKER_V2_DIR}/tests/json_common/mock/date_time_mock.c)

target_compile_options(app PRIVATE
	-DCONFIG_ASSET_TRACKER_V2_APP_VERSION_MAX_LEN=20
	-DEFTYPE=79
	-DCONFIG_LTE_NEIGHBOR_CELLS_MAX=10
)
