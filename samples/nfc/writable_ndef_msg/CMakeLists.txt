#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

set(NRF_SUPPORTED_BOARDS
  nrf52dk_nrf52832
  nrf52840dk_nrf52840
  nrf5340pdk_nrf5340_cpuapp
  nrf5340pdk_nrf5340_cpuappns
  )

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

FILE(GLOB app_sources src/*.c)
# NORDIC SDK APP START
target_sources(app  PRIVATE ${app_sources})
# NORDIC SDK APP END
