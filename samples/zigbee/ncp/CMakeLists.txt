#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# Set USB as default transport when nRF52 Dongle board is selected
if("${BOARD}" STREQUAL "nrf52840dongle_nrf52840")
  set(CONF_FILE "prj_usb.conf")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project("ZBOSS NCP SoC firmware")

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
)
# NORDIC SDK APP END
