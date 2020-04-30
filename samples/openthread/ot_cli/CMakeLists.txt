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

include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(openthread_cli)

target_sources(app PRIVATE src/main.c)
