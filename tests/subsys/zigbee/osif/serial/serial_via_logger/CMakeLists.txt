#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app)
add_definitions(-Wno-packed-bitfield-compat)

zephyr_compile_definitions(CONFIG_ZBOSS_TRACE_HEXDUMP_LOGGING)
zephyr_compile_definitions(CONFIG_ZBOSS_TRACE_LOGGER_BUFFER_SIZE=1024)

zephyr_include_directories(${NRFXLIB_DIR}/zboss/production/include)
zephyr_include_directories(${NRFXLIB_DIR}/zboss/production/include/osif)
target_include_directories(app BEFORE PRIVATE mock)

target_sources(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/zigbee/osif/zb_nrf_logger.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/zigbee/osif/zb_nrf_serial.c
  src/main.c
  mock/log_mock.c
)
