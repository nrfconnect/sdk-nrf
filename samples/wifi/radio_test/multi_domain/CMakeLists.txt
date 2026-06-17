#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_radio_test)

set(common_rt_dir ${ZEPHYR_NRF_MODULE_DIR}/samples/wifi/radio_test/common)
target_include_directories(app PRIVATE ${common_rt_dir}/inc)

target_sources(app PRIVATE
  src/nrf_wifi_radio_test_main.c
  ${common_rt_dir}/src/nrf_wifi_radio_test_shell.c
  ${common_rt_dir}/src/nrf_wifi_radio_ficr_shell.c
  ${common_rt_dir}/src/ficr_prog.c
)

target_link_libraries(app PRIVATE nrf-wifi-osal)
