#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

# Check if selected board is supported.
if(${BOARD} STREQUAL "nrf52_pca10040")
elseif(${BOARD} STREQUAL "nrf52840_pca10056")
else()
	message(FATAL_ERROR "board ${BOARD} is not supported")
endif()

FILE(GLOB app_sources src/*.c)
# NORDIC SDK APP START
target_sources(app  PRIVATE ${app_sources})
# NORDIC SDK APP END
