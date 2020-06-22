#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

set(NRF_SUPPORTED_BOARDS
  nrf5340pdk_nrf5340_cpuapp
  )

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(empty_app_core)

target_sources(app PRIVATE src/main.c)
