#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(location_test)

# Generate runner for the test
test_runner_generate(src/location_test.c)

cmock_handle(../../../include/modem/modem_key_mgmt.h)
cmock_handle(../../../include/net/rest_client.h)
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_modem_gnss.h)
cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_modem_at.h FUNC_EXCLUDE ".*nrf_modem_at_scanf")

zephyr_include_directories(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include)

target_sources(app PRIVATE src/location_test.c)

# Extend autoconf.h for nrf/lib/location
# Allows KConfig options to be enabled for nrf/lib/location without affecting CMake
# ..__nrf__lib__location is the name assigned to nrf/lib/location by the zephyr_library() macro in
# zephyr/cmake/modules/extensions.cmake
target_compile_options(..__nrf__lib__location PRIVATE "SHELL: -imacros ${PROJECT_SOURCE_DIR}/location_lib_autoconf_ext.h")
