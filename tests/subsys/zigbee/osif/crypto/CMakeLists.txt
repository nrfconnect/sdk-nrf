#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

add_definitions(-Wno-packed-bitfield-compat)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

zephyr_compile_definitions(CONFIG_ZBOSS_OSIF_LOG_LEVEL=LOG_LEVEL_DBG)

zephyr_include_directories(${NRF_DIR}/subsys/zigbee/osif)
zephyr_include_directories(${NRFXLIB_DIR}/zboss/include)
zephyr_include_directories(${NRFXLIB_DIR}/zboss/include/osif)
project(NONE)

target_sources(app PRIVATE
  src/main.c
  ${NRF_DIR}/subsys/zigbee/osif/zb_nrf_crypto.c
  ${NRF_DIR}/subsys/zigbee/osif/nrf_ecb_driver.c
)
