#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wifi_ble_radio_test)

set(common_rt_dir ${ZEPHYR_NRF_MODULE_DIR}/samples/wifi/radio_test/common)
set(perip_rt_dir ${ZEPHYR_NRF_MODULE_DIR}/samples/peripheral/radio_test)

if(CONFIG_NRF71_RADIO_TEST)
  target_link_libraries(app PRIVATE nrf71_wifi_radio_test_headers)
endif()

target_include_directories(app PRIVATE ${common_rt_dir}/inc)

# NORDIC SDK APP START
target_sources(app PRIVATE
  ${perip_rt_dir}/src/main.c
  ${perip_rt_dir}/src/radio_test.c
  ${perip_rt_dir}/src/radio_cmd.c
)

if(CONFIG_NRF71_RADIO_TEST)
  target_sources(app PRIVATE
    ${common_rt_dir}/src/nrf_wifi_radio_test_shell_nrf71.c
  )
else()
  target_sources(app PRIVATE
    ${common_rt_dir}/src/nrf_wifi_radio_test_shell.c
    ${common_rt_dir}/src/nrf_wifi_radio_ficr_shell.c
    ${common_rt_dir}/src/ficr_prog.c
  )

  target_link_libraries(app PRIVATE nrf-wifi-osal)
endif()
# NORDIC SDK APP END
