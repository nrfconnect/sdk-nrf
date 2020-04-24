#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(zigbee_osif_nvram_test)

add_definitions(-Wno-packed-bitfield-compat)
zephyr_compile_definitions(CONFIG_ZBOSS_OSIF_LOG_LEVEL=3)
zephyr_compile_definitions(ZB_PRODUCTION_CONFIG)

FILE(GLOB app_sources src/*.c)
target_sources(app
  PRIVATE
  ${app_sources}
  ${NRF_DIR}/subsys/zigbee/osif/zb_nrf_nvram.c
)

target_include_directories(app
  PRIVATE
  ${NRFXLIB_DIR}/zboss/include/osif
  ${NRFXLIB_DIR}/zboss/include
)
