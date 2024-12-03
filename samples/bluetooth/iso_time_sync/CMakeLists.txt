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

if (CONFIG_BT_ISO_BROADCASTER)
    target_sources(app PRIVATE src/bis_transmitter.c)
endif()

if (CONFIG_BT_ISO_SYNC_RECEIVER)
    target_sources(app PRIVATE src/bis_receiver.c)
endif()

if (CONFIG_BT_ISO_CENTRAL)
    target_sources(app PRIVATE src/cis_central.c)
endif()

if (CONFIG_BT_ISO_PERIPHERAL)
    target_sources(app PRIVATE src/cis_peripheral.c)
endif()

if (CONFIG_BT_ISO_TX_BUF_COUNT)
    target_sources(app PRIVATE src/iso_tx.c)
endif()

if (CONFIG_BT_ISO_RX_BUF_COUNT)
    target_sources(app PRIVATE src/iso_rx.c)
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
