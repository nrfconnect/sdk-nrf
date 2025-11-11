#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lte_lc_pdn)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stubs/lte_lc_stubs.c)

test_runner_generate(src/main.c)

target_sources(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/lte_lc.c
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/modules/pdn.c
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/modules/esm.c
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/modules/cellular_profile.c
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/common/event_handler_list.c
)

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/stubs)
zephyr_include_directories(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/)
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/include/modem/)
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/)
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/include)
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/include/modules/)
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/include/common/)
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/lib/at_monitor/)
zephyr_include_directories(${ZEPHYR_NRF_MODULE_DIR}/lib/at_parser/)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/testsuite/include)

add_compile_definitions(CONFIG_LTE_LC_PDN_MODULE=1)
add_compile_definitions(CONFIG_LTE_LC_PDN_HEAP_SIZE=64)
add_compile_definitions(CONFIG_LTE_LC_PDN_ESM_TIMEOUT=1000)
add_compile_definitions(CONFIG_LTE_LC_PDN_DEFAULTS_OVERRIDE)
add_compile_definitions(CONFIG_LTE_LC_PDN_DEFAULT_APN="apn0")
add_compile_definitions(CONFIG_LTE_LC_PDN_DEFAULT_FAM=2)
add_compile_definitions(CONFIG_LTE_LC_PDN_ESM_STRERROR)
add_compile_definitions(CONFIG_HEAP_MEM_POOL_ADD_SIZE_LTE_LC_PDN=64)
add_compile_definitions(CONFIG_LTE_LINK_CONTROL)
add_compile_definitions(CONFIG_LTE_LC_CELLULAR_PROFILE_MODULE=1)
add_compile_definitions(CONFIG_LTE_LINK_CONTROL_LOG_LEVEL=0)
add_compile_definitions(CONFIG_AT_MONITOR_LOG_LEVEL=0)
add_compile_definitions(CONFIG_HEAP_MEM_POOL_ADD_SIZE_LTE_LINK_CONTROL=96)
