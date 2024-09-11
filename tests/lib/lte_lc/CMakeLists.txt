#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lte_lc)

test_runner_generate(src/main.c)

cmock_handle(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include/nrf_modem_at.h
	     FUNC_EXCLUDE ".*nrf_modem_at_scanf"
	     FUNC_EXCLUDE ".*nrf_modem_at_printf")

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

FILE(GLOB lib_module_sources ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/modules/*.c)
FILE(GLOB lib_common_sources ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/common/*.c)

target_sources(app
  PRIVATE
  ${lib_module_sources}
  ${lib_common_sources}
)

target_include_directories(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/
  ${ZEPHYR_NRF_MODULE_DIR}/lib/lte_link_control/include
  ${ZEPHYR_NRF_MODULE_DIR}/include/modem/
  ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include
)

target_compile_options(app
  PRIVATE
  -DCONFIG_LTE_LINK_CONTROL_LOG_LEVEL=0
  -DCONFIG_LTE_NEIGHBOR_CELLS_MAX=10
  -DCONFIG_LTE_LC_WORKQUEUE_STACK_SIZE=512
  -DCONFIG_LTE_LC_TAU_PRE_WARNING_TIME_MS=0
  -DCONFIG_LTE_LC_MODEM_SLEEP_PRE_WARNING_TIME_MS=0
  -DCONFIG_LTE_LC_MODEM_SLEEP_NOTIFICATIONS_THRESHOLD_MS=0
  -DCONFIG_LTE_LC_TAU_PRE_WARNING_THRESHOLD_MS=0
  -DCONFIG_LTE_MODE_PREFERENCE_VALUE=0
  -DCONFIG_LTE_PSM_REQ_RAT="00100001"
  -DCONFIG_LTE_PSM_REQ_RPTAU="00000011"
  -DCONFIG_LTE_NETWORK_TIMEOUT=0
  -DCONFIG_LTE_EDRX_REQ_VALUE_LTE_M="1001"
  -DCONFIG_LTE_EDRX_REQ_VALUE_NBIOT="1001"
  -DCONFIG_LTE_PTW_VALUE_LTE_M=""
  -DCONFIG_LTE_PTW_VALUE_NBIOT=""
  -DCONFIG_LTE_LC_CONN_EVAL=1
  -DCONFIG_LTE_LC_EDRX=1
  -DCONFIG_LTE_LC_NEIGHBOR_CELL_MEAS=1
  -DCONFIG_LTE_LC_PERIODIC_SEARCH=1
  -DCONFIG_LTE_LC_PSM=1
  -DCONFIG_LTE_LC_RAI=1
  -DCONFIG_LTE_LC_REDMOB=1
  -DCONFIG_LTE_LC_FACTORY_RESET=1
  -DCONFIG_LTE_LC_MODEM_SLEEP=1
  -DCONFIG_LTE_LC_TAU_PRE_WARNING=1
)
