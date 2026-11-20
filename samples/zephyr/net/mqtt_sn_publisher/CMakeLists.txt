#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mqtt_sn_publisher)

target_sources(app PRIVATE
  ${ZEPHYR_BASE}/samples/net/mqtt_sn_publisher/src/main.c
  ${ZEPHYR_BASE}/samples/net/mqtt_sn_publisher/src/udp.c
)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
