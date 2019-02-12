#
# Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

# Check if selected board is supported.
if(${BOARD} STREQUAL "nrf52840_pca10056")
	set(CONF_FILE nrf52840_pca10056.conf)
elseif(${BOARD} STREQUAL "nrf9160_pca10090")
	set(CONF_FILE nrf9160_pca10090.conf)
else()
	message(FATAL_ERROR "board ${BOARD} is not supported")
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(adafruit_lcd)

FILE(GLOB app_sources src/*.c)
target_sources(app  PRIVATE ${app_sources})
