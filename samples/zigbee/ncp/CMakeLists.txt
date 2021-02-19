#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

# Increase size of MCUboot partition when building with MCUboot
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static.yml"
    AND "prj_usb.conf" IN_LIST CONF_FILE)
  set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static.yml
  )
endif()

# Include pm_static file to build for nRF52 dongle with nRF5 Bootloader
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static.yml"
    AND ${BOARD} STREQUAL "nrf52840dongle_nrf52840")
  set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static.yml
  )
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project("ZBOSS NCP SoC firmware")

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
)
# NORDIC SDK APP END
