#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(lte-ble-gateway)

target_sources(app PRIVATE src/main.c)
zephyr_include_directories(src)

include_directories(src/orientation_detector)
target_sources(app PRIVATE src/aggregator.c)
target_sources(app PRIVATE src/alarm.c)
target_sources(app PRIVATE src/ble.c)
