#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lte_lc_pdn)

target_sources(app PRIVATE
  src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs/lte_lc_stubs.c
)

test_runner_generate(src/main.c)

target_sources(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/lte_lc.c
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/modules/pdn.c
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/modules/esm.c
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/modules/cellular_profile.c
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/common/event_handler_list.c
)

zephyr_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/stubs
  ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/
  ${ZEPHYR_NRF_MODULE_DIR}/include/modem/
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/include
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/include/modules/
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/include/common/
  ${ZEPHYR_NRF_MODULE_DIR}/lib/at_monitor/
  ${ZEPHYR_NRF_MODULE_DIR}/lib/at_parser/
  ${ZEPHYR_BASE}/subsys/testsuite/include
)

add_compile_definitions(
  CONFIG_LTE_LC_PDN_MODULE=1
  CONFIG_LTE_LC_PDN_ESM_TIMEOUT=1000
  CONFIG_LTE_LC_PDN_DEFAULTS_OVERRIDE
  CONFIG_LTE_LC_PDN_DEFAULT_APN="apn0"
  CONFIG_LTE_LC_PDN_DEFAULT_FAM=2
  CONFIG_LTE_LC_PDN_ESM_STRERROR
  CONFIG_LTE_LINK_CONTROL
  CONFIG_LTE_LC_CELLULAR_PROFILE_MODULE=1
  CONFIG_LTE_LINK_CONTROL_LOG_LEVEL=0
  CONFIG_AT_MONITOR_LOG_LEVEL=0
  CONFIG_HEAP_MEM_POOL_ADD_SIZE_LTE_LINK_CONTROL=96
)
