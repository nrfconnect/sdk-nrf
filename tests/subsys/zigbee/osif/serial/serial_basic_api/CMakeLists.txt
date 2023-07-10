#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(app)
add_definitions(-Wno-packed-bitfield-compat)

# Add the serial definition here, so we can test ZBOSS'es Kconfig option without including ZBOSS libraries
zephyr_compile_definitions(CONFIG_ZB_HAVE_SERIAL)
zephyr_compile_definitions(CONFIG_ZIGBEE_HAVE_SERIAL)
zephyr_compile_definitions(CONFIG_ZIGBEE_HAVE_ASYNC_SERIAL)
zephyr_compile_definitions(CONFIG_ZBOSS_TRACE_BINARY_NCP_TRANSPORT_LOGGING)
zephyr_compile_definitions(CONFIG_ZIGBEE_UART_TX_TIMEOUT=10)
zephyr_compile_definitions(CONFIG_ZIGBEE_UART_RX_TIMEOUT=10)
zephyr_compile_definitions(CONFIG_ZIGBEE_UART_PARTIAL_RX_TIMEOUT=10)
zephyr_compile_definitions(CONFIG_ZIGBEE_UART_PARTIAL_TX_TIMEOUT=10)
zephyr_compile_definitions(CONFIG_ZIGBEE_UART_RX_BUF_LEN=16)
zephyr_compile_definitions(CONFIG_ZIGBEE_UART_TX_BUF_LEN=128)

zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/subsys/zigbee/osif)
zephyr_include_directories(${NRFXLIB_DIR}/zboss/production/include)
zephyr_include_directories(${NRFXLIB_DIR}/zboss/production/include/osif)


target_sources(app PRIVATE
  src/main.c
  mock/zigbee_scheduler.c
)
