#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(jwt_test)

# generate runner for the test
test_runner_generate(src/jwt_test.c)

target_include_directories(app PRIVATE src)

cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_modem_at.h
	     WORD_EXCLUDE "__nrf_modem_(printf|scanf)_like\(.*\)")

# add test file
target_sources(app PRIVATE src/jwt_test.c)
target_sources(app PRIVATE ../../../lib/modem_jwt/modem_jwt.c)
add_definitions(-DCONFIG_MODEM_JWT_MAX_LEN=850)

target_include_directories(app PRIVATE ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include)
