#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

target_compile_options(app PRIVATE -Wno-packed-bitfield-compat)

target_compile_definitions(app PRIVATE
  CONFIG_ZBOSS_OSIF_LOG_LEVEL=LOG_LEVEL_DBG
)

target_include_directories(app PRIVATE
  ${NRF_DIR}/subsys/zigbee/osif
  ${NRFXLIB_DIR}/zboss/include
  ${NRFXLIB_DIR}/zboss/include/osif
)

project(osif_crypto)

target_sources(app PRIVATE
  src/main.c
  ${NRF_DIR}/subsys/zigbee/osif/zb_nrf_crypto.c
)
