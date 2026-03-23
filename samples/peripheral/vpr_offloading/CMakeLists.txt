#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(vpr_offloading)

target_sources(app PRIVATE src/main.c common/ipc_comm.c)

if(CONFIG_SAMPLE_LOCAL_TEMP_MONITOR)
  target_sources(app PRIVATE common/temp_monitor.c)
else()
  target_sources(app PRIVATE src/remote_temp_monitor.c)
endif()

if(CONFIG_SAMPLE_LOCAL_LED_CONTROL)
  target_sources(app PRIVATE common/led_control.c)
else()
  target_sources(app PRIVATE src/remote_led_control.c)
endif()
