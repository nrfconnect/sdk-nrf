#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gps_module_test)

# generate runner for the test
test_runner_generate(src/gps_module_test.c)

# create mock
cmock_handle(../../src/modules/modules_common.h )
cmock_handle(${ZEPHYR_BASE}/../nrf/include/event_manager.h )
cmock_handle(${ZEPHYR_BASE}/../nrf/include/date_time.h )
cmock_handle(${ZEPHYR_BASE}/../nrf/include/drivers/gps.h drivers )
cmock_handle(${ZEPHYR_BASE}/include/sys/reboot.h sys)


# add gps_module (the unit under test)
target_sources(app PRIVATE ../../src/modules/gps_module.c)

# add test file
target_sources(app PRIVATE src/gps_module_test.c)

target_include_directories(app PRIVATE .)
target_include_directories(app PRIVATE ../../src/)
target_include_directories(app PRIVATE ../../src/modules/)
target_include_directories(app PRIVATE ../../src/events/)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/../nrf/subsys/event_manager)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/../nrf/modules/cjson/include)

# Options that cannot be passed through Kconfig fragments.
target_compile_options(app PRIVATE
  	-DCONFIG_ASSET_TRACKER_V2_APP_VERSION_MAX_LEN=20
	-DCONFIG_GPS_DEV_NAME="GPS_SIM"
	-DCONFIG_GPS_MODULE_NMEA)
