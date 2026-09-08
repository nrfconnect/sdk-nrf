#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# DECT NR+ Stack Integration Tests with Unity and CMock

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(dect_mdm_integration_test)

target_sources(app PRIVATE
    src/main.c
    src/test_dect_integration.c
    src/test_dect_eth_integration.c
    src/test_dect_utils.c
    src/test_dect_sink_uplink.c
    src/mocks/mock_nrf_modem_dect_mac.c
)

target_sources_ifdef(CONFIG_MODEM_CELLULAR app PRIVATE
    src/mocks/mock_modem_device.c
    src/mocks/mock_ppp_net_if.c
)

target_sources_ifdef(CONFIG_NET_L2_ETHERNET app PRIVATE
    src/mocks/mock_eth_net_if.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mocks
    ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include
    ${ZEPHYR_NRF_MODULE_DIR}/drivers/dect/dect_mdm
    ${ZEPHYR_NRF_MODULE_DIR}/include/net/dect
    ${ZEPHYR_NRF_MODULE_DIR}/include/net
    ${ZEPHYR_BASE}/include/zephyr/net
    ${ZEPHYR_BASE}/subsys/net/ip
)

target_include_directories(zephyr_interface INTERFACE
    ${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include
)

if(CONFIG_MODEM_CELLULAR)
  target_link_options(app PUBLIC
    -Wl,--wrap=net_if_get_first_by_type
    -Wl,--wrap=net_if_start_rs
  )
endif()

if(CONFIG_NET_L2_ETHERNET)
  target_link_options(app PUBLIC
    -Wl,--wrap=net_if_start_rs
    -Wl,--wrap=net_ipv6_send_na
    -Wl,--wrap=net_ipv6_send_ns
  )
endif()
