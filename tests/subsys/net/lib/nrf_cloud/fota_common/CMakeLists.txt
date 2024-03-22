#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_fota_common_test)
set(NRF_SDK_DIR ${ZEPHYR_BASE}/../nrf)
cmake_path(NORMAL_PATH NRF_SDK_DIR)

if (CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)
FILE(GLOB app_sources src/fmfu.c)
else()
FILE(GLOB app_sources src/main.c)
endif()
target_sources(app PRIVATE ${app_sources})

if (NOT CONFIG_NRF_CLOUD_FOTA)
	target_sources(app
		PRIVATE
		${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/src/nrf_cloud_fota_common.c
	)
endif()

target_include_directories(app
	PRIVATE
	src
	${NRF_SDK_DIR}/subsys/net/lib/nrf_cloud/include
	${NRF_SDK_DIR}/subsys/net/lib/nrf_cloud/src
	${ZEPHYR_BASE}/subsys/testsuite/include
)

if (CONFIG_FOTA_DOWNLOAD)
set_source_files_properties(
	${NRF_SDK_DIR}/subsys/net/lib/fota_download/src/fota_download.c
	DIRECTORY ${NRF_SDK_DIR}/subsys/net/lib/fota_download/
	PROPERTIES HEADER_FILE_ONLY ON
)
endif()

if (CONFIG_MCUBOOT_IMG_MANAGER)
set_source_files_properties(
	${ZEPHYR_BASE}/subsys/dfu/boot/mcuboot.c
	DIRECTORY ${ZEPHYR_BASE}/subsys/dfu/boot/
	PROPERTIES HEADER_FILE_ONLY ON
)
endif()

if (CONFIG_NRF_CLOUD_FOTA_FULL_MODEM_UPDATE)
set_source_files_properties(
	${NRF_SDK_DIR}/subsys/dfu/dfu_target/src/dfu_target_full_modem.c
	DIRECTORY ${NRF_SDK_DIR}/subsys/dfu/dfu_target/
	PROPERTIES HEADER_FILE_ONLY ON
)

set_source_files_properties(
	${NRF_SDK_DIR}/subsys/dfu/fmfu_fdev/src/fmfu_fdev.c
	DIRECTORY ${NRF_SDK_DIR}/subsys/dfu/fmfu_fdev/
	PROPERTIES HEADER_FILE_ONLY ON
)

set_source_files_properties(
	${NRF_SDK_DIR}/lib/nrf_modem_lib/nrf_modem_lib.c
	DIRECTORY ${NRF_SDK_DIR}/lib/nrf_modem_lib/
	PROPERTIES HEADER_FILE_ONLY ON
)
endif()
