#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wifi_ble_radio_test)

set(MULTI_DOMAIN_RT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../multi_domain)
set(PERIP_RT_DIR ${ZEPHYR_NRF_MODULE_DIR}/samples/peripheral/radio_test)

target_include_directories(app PRIVATE ${MULTI_DOMAIN_RT_DIR}/inc)

# NORDIC SDK APP START
target_sources(app PRIVATE
  ${PERIP_RT_DIR}/src/main.c
  ${PERIP_RT_DIR}/src/radio_test.c
  ${PERIP_RT_DIR}/src/radio_cmd.c
  ${MULTI_DOMAIN_RT_DIR}/src/nrf_wifi_radio_test_shell.c
  ${MULTI_DOMAIN_RT_DIR}/src/nrf_wifi_radio_ficr_shell.c
  ${MULTI_DOMAIN_RT_DIR}/src/ficr_prog.c
)
# NORDIC SDK APP END

target_link_libraries(app PRIVATE nrf-wifi-osal)
