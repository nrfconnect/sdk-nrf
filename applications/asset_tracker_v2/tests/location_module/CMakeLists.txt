#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(location_module_test)

# generate runner for the test
test_runner_generate(src/location_module_test.c)

# create mock
cmock_handle(../../src/modules/modules_common.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/app_event_manager.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/subsys/app_event_manager/app_event_manager_priv.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/date_time.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/modem/location.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/modem/lte_lc.h
	FUNC_EXCLUDE ".*(lte_lc_rai_req|lte_lc_rai_param_set)"
	WORD_EXCLUDE "__deprecated")
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_modem_gnss.h)

# add location_module (the unit under test)
target_sources(app PRIVATE ../../src/modules/location_module.c)

# add test file
target_sources(app PRIVATE src/location_module_test.c)

target_include_directories(app PRIVATE .)
target_include_directories(app PRIVATE ../../src/)
target_include_directories(app PRIVATE ../../src/modules/)
target_include_directories(app PRIVATE ../../src/events/)
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/subsys/app_event_manager)
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/modules/cjson/include)

# Options that cannot be passed through Kconfig fragments.
target_compile_options(app PRIVATE
	-DCONFIG_LOCATION_METHODS_LIST_SIZE=3
	-DCONFIG_LOCATION_DATA_DETAILS=y
	-DCONFIG_ASSET_TRACKER_V2_APP_VERSION_MAX_LEN=20
	-DCONFIG_CLOUD_CODEC_APN_LEN_MAX=1
	-DCONFIG_MODEM_APN_LEN_MAX=1
	-DCONFIG_CLOUD_CODEC_LWM2M_PATH_LIST_ENTRIES_MAX=1
	-DCONFIG_CLOUD_CODEC_LWM2M_PATH_ENTRY_SIZE_MAX=1
	-DCONFIG_LTE_NEIGHBOR_CELLS_MAX=10
	-DCONFIG_LOCATION_SERVICE_EXTERNAL=y
	-DCONFIG_LOCATION_METHOD_CELLULAR=y
	-DCONFIG_NRF_CLOUD_AGNSS=y
	-DCONFIG_AT_MONITOR_HEAP_SIZE=1024
)
