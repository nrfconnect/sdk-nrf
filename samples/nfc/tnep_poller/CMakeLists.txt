#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

# Set supported board
set(NRF_SUPPORTED_BOARDS
  nrf52dk_nrf52832
  nrf52840dk_nrf52840
  nrf5340pdk_nrf5340_cpuapp
  )

include(../../../cmake/boilerplate.cmake)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

FILE(GLOB app_sources src/*.c)
# NORDIC SDK APP START
target_sources(app  PRIVATE ${app_sources})
# NORDIC SDK APP END

# Include configuration headers
zephyr_library_include_directories(configuration)
