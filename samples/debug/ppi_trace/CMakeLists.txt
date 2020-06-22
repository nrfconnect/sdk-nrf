#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("PPI trace sample")

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(
	CONFIG_USE_BLUETOOTH_RADIO_EVENTS
	app PRIVATE src/bluetooth.c
)
