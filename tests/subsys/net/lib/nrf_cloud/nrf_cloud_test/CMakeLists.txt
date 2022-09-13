#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_test)
set(NRF_SDK_DIR ${ZEPHYR_BASE}/../nrf)
cmake_path(NORMAL_PATH NRF_SDK_DIR)

# NRF CLOUD TEST START
FILE(GLOB app_sources src/nrf_cloud_test.c)
target_sources(app PRIVATE ${app_sources})

target_sources(app
	PRIVATE
	${NRF_SDK_DIR}/subsys/net/lib/nrf_cloud/src/nrf_cloud.c
)

target_include_directories(app
	PRIVATE
	src
	${NRF_SDK_DIR}/subsys/net/lib/nrf_cloud/include
	${ZEPHYR_BASE}/subsys/testsuite/include
	${ZEPHYR_BASE}/../modules/lib/cjson
)

if (CONFIG_NRF_CLOUD_MQTT OR CONFIG_NRF_CLOUD_FOTA)

	# These code files are mocked and not necessary to be included,
	# also prevent nrf_cloud.c to be included multiple times by KConfig
	set_source_files_properties(
		${NRF_SDK_DIR}/subsys/net/lib/nrf_cloud/src/nrf_cloud.c
		${NRF_SDK_DIR}/subsys/net/lib/nrf_cloud/src/nrf_cloud_fota.c
		${NRF_SDK_DIR}/subsys/net/lib/nrf_cloud/src/nrf_cloud_fota_common.c
		${NRF_SDK_DIR}/subsys/net/lib/nrf_cloud/src/nrf_cloud_codec.c
		${NRF_SDK_DIR}/subsys/net/lib/nrf_cloud/src/nrf_cloud_fsm.c
		${NRF_SDK_DIR}/subsys/net/lib/nrf_cloud/src/nrf_cloud_transport.c
		DIRECTORY ${NRF_SDK_DIR}/subsys/net/lib/nrf_cloud/
		PROPERTIES HEADER_FILE_ONLY ON
	)

endif()
# NRF CLOUD TEST END
