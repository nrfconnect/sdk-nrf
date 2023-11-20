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
  ${ZEPHYR_NRF_MODULE_DIR}/lib/hw_id/hw_id.c
)

zephyr_include_directories(src/)
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/include/)
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/include/net)
zephyr_include_directories(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/)
zephyr_include_directories(${ZEPHYR_BASE}/include/zephyr/)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/testsuite/include)

if(KCONFIG_OVERRIDE_FILE)
  add_definitions(-include ${KCONFIG_OVERRIDE_FILE})
elseif(CONFIG_HW_ID_OVERRIDE_FILE)
  add_definitions(-include ${CONFIG_HW_ID_OVERRIDE_FILE})
endif()
