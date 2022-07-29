#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf_dm)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/pwm_led.c
  src/peer.c
  src/service.c
  )
# NORDIC SDK APP END

zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

target_link_options(app PUBLIC "-Wl,--no-wchar-size-warning")
