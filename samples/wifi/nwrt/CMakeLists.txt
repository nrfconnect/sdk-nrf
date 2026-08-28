#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# west build -- -Dapp_EXTRA_CONF_FILE=<path>/prj_swd_only.conf
set(NWRT_SWD_ONLY_BUILD OFF CACHE BOOL "No Wi-Fi driver; shadow protocol test only")

if(NWRT_SWD_ONLY_BUILD)
  list(APPEND EXTRA_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/prj_swd_only.conf)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nwrt)

set(NRF71_WIFI ${ZEPHYR_NRF_MODULE_DIR}/drivers/wifi/nrf71)

if(NOT CONFIG_NWRT_SWD_ONLY)
  include(${WEST_TOPDIR}/test_utilities/nrf71/phy/radio_test/extn_cmds/CMakeLists.txt)
endif()

target_include_directories(app PRIVATE include)

target_sources(app PRIVATE
  src/main.c
  src/nwrt_shadow.c
)

if(CONFIG_NWRT_SWD_ONLY)
  target_sources(app PRIVATE src/nwrt_conf_mock.c)
  target_include_directories(app PRIVATE
    ${NRF71_WIFI}/osal/os_if/inc
    ${NRF71_WIFI}/osal/fw_if/umac_if/inc
    ${NRF71_WIFI}/osal/hw_if/hal/inc
    ${NRF71_WIFI}/osal/bus_if/bal/inc
    ${NRF71_WIFI}/osal/bus_if/bus/qspi/inc
    ${NRF71_WIFI}/utils/inc
    ${NRF71_WIFI}/fw_if
    ${ZEPHYR_NRF_MODULE_DIR}/include/drivers/wifi/nrf71
  )
else()
  target_sources(app PRIVATE src/nwrt_conf.c)
  target_link_libraries(app PRIVATE
    nrf71_wifi_radio_test_headers
    test_utilities_radio_test_extn_headers
  )
endif()
