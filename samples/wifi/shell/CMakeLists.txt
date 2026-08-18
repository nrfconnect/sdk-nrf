#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_wifi_shell)

if(CONFIG_NRF71_RAW_DATA_TX OR CONFIG_NRF71_PROMISC_DATA_RX)
  target_include_directories(app PUBLIC ${ZEPHYR_BASE}/subsys/net/ip)
endif()

target_sources_ifdef(CONFIG_NRF70_RAW_DATA_TX app PRIVATE src/wifi_raw_tx_pkt_shell.c)

target_sources_ifdef(CONFIG_NRF70_PROMISC_DATA_RX app PRIVATE src/wifi_promiscuous_shell.c)
target_sources_ifdef(CONFIG_NRF71_PROMISC_DATA_RX app PRIVATE src/wifi_promiscuous_shell.c)

target_sources(app PRIVATE src/main.c)
