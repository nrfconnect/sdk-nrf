#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zigbee_osif_test)

zephyr_compile_definitions(CONFIG_ZBOSS_OSIF_LOG_LEVEL=3)

FILE(GLOB app_sources src/*.c)
target_sources(app
  PRIVATE
  ${app_sources}
  ${NRF_DIR}/subsys/zigbee/osif/zb_nrf_timer.c
)

target_include_directories(app
  PRIVATE
  ${NRF_DIR}/tests/subsys/zigbee/osif/timer/mock
  ${NRF_DIR}/tests/subsys/zigbee/osif/timer/stubs
  ${NRFXLIB_DIR}/zboss/include/osif
)
