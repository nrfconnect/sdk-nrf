#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gatt_dm)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE
  mock/gatt_discover_mock.c
  ${app_sources}
)
