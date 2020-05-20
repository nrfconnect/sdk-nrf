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

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(openthread_coap_client)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

target_include_directories(app PRIVATE ../coap_server/interface)
