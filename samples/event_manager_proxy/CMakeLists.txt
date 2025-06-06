#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(event_manager_proxy)

add_subdirectory(common_events)
add_subdirectory(modules)

target_sources(app PRIVATE src/main.c)
if(CONFIG_SOC_NRF54H20_IRON)
  target_sources(app PRIVATE periphconf_migrated_src/periphconf_migrated_from_uicr.c)
endif()
