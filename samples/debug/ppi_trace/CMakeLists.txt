#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("PPI trace sample")

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(
	CONFIG_USE_BLUETOOTH_RADIO_EVENTS
	app PRIVATE src/bluetooth.c
)
