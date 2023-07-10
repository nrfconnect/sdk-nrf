#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ui_module_test)

# Generate runner for the test
test_runner_generate(src/ui_module_test.c)

# Set CMake path variables for convenience
set(ASSET_TRACKER_V2_DIR ../..)

# Create mocks
cmock_handle(${ASSET_TRACKER_V2_DIR}/src/modules/modules_common.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/app_event_manager.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/subsys/app_event_manager/app_event_manager_priv.h)
cmock_handle(${ZEPHYR_NRF_MODULE_DIR}/include/dk_buttons_and_leds.h)

# Include target sources
target_sources(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/modules/ui_module.c)
target_sources(app PRIVATE src/ui_module_test.c)

target_include_directories(app PRIVATE .)
target_include_directories(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/)
target_include_directories(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/modules/)
target_include_directories(app PRIVATE ${ASSET_TRACKER_V2_DIR}/src/events/)
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/subsys/app_event_manager)
target_include_directories(app PRIVATE ${ZEPHYR_NRF_MODULE_DIR}/modules/cjson/include)
target_include_directories(app PRIVATE ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/)

# Options that cannot be passed through Kconfig fragments
target_compile_definitions(app PRIVATE
	-DCONFIG_ASSET_TRACKER_V2_APP_VERSION_MAX_LEN=20
	-DCONFIG_CLOUD_CODEC_APN_LEN_MAX=1
	-DCONFIG_MODEM_APN_LEN_MAX=1
	-DCONFIG_CLOUD_CODEC_LWM2M_PATH_LIST_ENTRIES_MAX=1
	-DCONFIG_CLOUD_CODEC_LWM2M_PATH_ENTRY_SIZE_MAX=1
	-DCONFIG_LTE_NEIGHBOR_CELLS_MAX=10
)

# Include ui_module_test.h that re-defines SYS_INIT() for unit testing purposes.
set_property(SOURCE ${ASSET_TRACKER_V2_DIR}/src/modules/ui_module.c PROPERTY COMPILE_FLAGS
	     "-include src/ui_module_test.h")
