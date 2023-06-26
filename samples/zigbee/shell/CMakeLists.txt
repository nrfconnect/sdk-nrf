#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# Set USB as default transport when sample is built for the nRF52 Dongle
if("${BOARD}" STREQUAL "nrf52840dongle_nrf52840")
  set(CONF_FILE "prj_usb.conf")
endif()

# Include app_usb.overlay when sample is built with prj_usb.conf
if("${CONF_FILE}" STREQUAL "prj_usb.conf")
  set(DTC_OVERLAY_FILE "app_usb.overlay")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project("Zigbee application with Zigbee shell")

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
)

target_include_directories(app PRIVATE include)
# NORDIC SDK APP END
