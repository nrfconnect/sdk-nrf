#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_mqtt_device_message_sample)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/nrf_cloud_shadow.c)

zephyr_include_directories(src)
