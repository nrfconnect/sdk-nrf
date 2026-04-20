#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
# Board/shield and overlay lookup in original directory
set(APPLICATION_CONFIG_DIR "\${ZEPHYR_BASE}/samples/net/mqtt_sn_publisher")
# Point to original prj.conf (add one more file here if you need overrides)
set(CONF_FILE "\${ZEPHYR_BASE}/samples/net/mqtt_sn_publisher/prj.conf")
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mqtt_sn_publisher)

target_sources(app PRIVATE
  ${ZEPHYR_BASE}/samples/net/mqtt_sn_publisher/src/main.c
  ${ZEPHYR_BASE}/samples/net/mqtt_sn_publisher/src/udp.c
)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
