#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_fota_common_test)
set(NRF_SDK_DIR ${ZEPHYR_BASE}/../nrf)
cmake_path(NORMAL_PATH NRF_SDK_DIR)

FILE(GLOB app_sources src/main.c)

target_sources(app PRIVATE ${app_sources})

target_sources(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/net/lib/nrf_cloud/src/nrf_cloud_fota.c
)

target_include_directories(app
	PRIVATE
	src
	${NRF_SDK_DIR}/subsys/net/lib/nrf_cloud/include
	${NRF_SDK_DIR}/subsys/net/lib/nrf_cloud/src
	${ZEPHYR_BASE}/subsys/testsuite/include
)

target_compile_options(app
  PRIVATE
  -DCONFIG_DOWNLOAD_CLIENT_BUF_SIZE=2048
  -DCONFIG_DOWNLOAD_CLIENT_STACK_SIZE=1280
  -DCONFIG_NRF_CLOUD_FOTA_PROGRESS_PCT_INCREMENT=10
  -DCONFIG_NRF_CLOUD_FOTA_DOWNLOAD_FRAGMENT_SIZE=1700
  -DCONFIG_NRF_CLOUD_FOTA_BLE_DEVICES=1
  -DCONFIG_NRF_CLOUD_FOTA_SMP=1
  -DUNIT_TESTING=1
  -DEFTYPE=79
)
