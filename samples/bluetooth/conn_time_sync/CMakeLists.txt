#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(iso_time_sync)

target_include_directories(app PRIVATE include)
target_sources(app PRIVATE src/main.c src/timed_led_toggle.c)

if (CONFIG_BT_CENTRAL)
    target_sources(app PRIVATE src/central.c)
endif()

if (CONFIG_BT_PERIPHERAL)
    target_sources(app PRIVATE src/peripheral.c)
endif()

if (CONFIG_SOC_COMPATIBLE_NRF52X)
    target_sources(app PRIVATE src/controller_time_nrf52.c)
elseif (CONFIG_SOC_COMPATIBLE_NRF5340_CPUAPP)
    target_sources(app PRIVATE src/controller_time_nrf53_app.c)
elseif (CONFIG_SOC_SERIES_NRF54LX OR CONFIG_SOC_SERIES_NRF54HX)
    target_sources(app PRIVATE src/controller_time_nrf54.c)
else()
    MESSAGE(FATAL_ERROR "Unsupported series")
endif()
