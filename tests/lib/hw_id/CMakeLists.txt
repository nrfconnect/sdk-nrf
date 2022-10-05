#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hw_id_test)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

test_runner_generate(src/main.c)

target_sources(app
  PRIVATE
  ${ZEPHYR_BASE}/../nrf/lib/hw_id/hw_id.c
)

zephyr_include_directories(src/)
zephyr_include_directories(${ZEPHYR_BASE}/../nrf/include/)
zephyr_include_directories(${ZEPHYR_BASE}/../nrf/include/net)
zephyr_include_directories(${ZEPHYR_BASE}/../nrfxlib/nrf_modem/include/)
zephyr_include_directories(${ZEPHYR_BASE}/include/zephyr/)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/testsuite/include)

if(KCONFIG_OVERRIDE_FILE)
  add_definitions(-include ${KCONFIG_OVERRIDE_FILE})
endif()
