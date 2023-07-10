#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

target_compile_options(app PRIVATE -Wno-packed-bitfield-compat)

target_compile_definitions(app PRIVATE
  CONFIG_ZBOSS_OSIF_LOG_LEVEL=LOG_LEVEL_DBG
)

# CONFIG_ZIGBEE_USE_SOFTWARE_AES is defined in CMakeLists
# because of unsatisfied dependency to CONFIG_ZIGBEE.
# nrfxlib_crypto is linked in here because it can't be linked
# in Zigbee subsys CMakeLists.txt.

if (${BOARD} STREQUAL "nrf5340dk_nrf5340_cpuapp")
  target_compile_definitions(app PRIVATE
  CONFIG_ZIGBEE_USE_SOFTWARE_AES=1
)
  zephyr_link_libraries(nrfxlib_crypto)
endif()

target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/zigbee/osif
  ${NRFXLIB_DIR}/zboss/production/include
  ${NRFXLIB_DIR}/zboss/production/include/osif
)

project(osif_crypto)

target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/zigbee/osif/zb_nrf_crypto.c
)
