#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/../nrf/cmake/boilerplate.cmake)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(json)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Get direct access to the API provided by dfu_target_flash
target_include_directories(app
  PRIVATE
  ${ZEPHYR_BASE}/../nrf/subsys/dfu/include
  )
