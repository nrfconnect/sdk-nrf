#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
cmake_minimum_required(VERSION 3.13.1)

set(NRF_SUPPORTED_BOARDS
	nrf52840dk_nrf52840
	nrf52833dk_nrf52833
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(openthread_coap_client)

target_sources(app PRIVATE src/coap_client.c
			   src/coap_client_utils.c)

target_include_directories(app PUBLIC ../coap_server/interface)
