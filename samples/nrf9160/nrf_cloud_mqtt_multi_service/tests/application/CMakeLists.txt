#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_cloud_mqtt_multi_service_test_application)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} src/main.c)

target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ../../src/
				       ${CMAKE_CURRENT_SOURCE_DIR} ../../../../../subsys/net/lib/nrf_cloud/include/)
