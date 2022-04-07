#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

set(ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR})
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Sensor Aggregator sample")

if(NOT CONFIG_EVENT_MANAGER_PROXY)
  target_sources(app PRIVATE src/modules/sensor_sim_ctrl.c)
else()
  target_sources(app PRIVATE configuration/common/event_proxy_init.c)
endif()

# Include application application event headers
zephyr_library_include_directories(src/events)

target_sources(app PRIVATE
  src/main.c
  src/modules/workload_sim.c)

# Add include directory for board specific CAF def files
zephyr_include_directories(
  configuration/common
  configuration/${BOARD})
