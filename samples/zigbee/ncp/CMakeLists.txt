#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)

if ("usb.overlay" IN_LIST OVERLAY_CONFIG OR ${BOARD} STREQUAL "nrf52840dongle_nrf52840")
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static.yml")
    set(PM_STATIC_YML_FILE
      ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/pm_static.yml
    )
  endif()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/mcuboot.conf")
    set(mcuboot_OVERLAY_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/mcuboot.conf)
  endif()

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/app_with_mcuboot.conf")
    set(CONF_FILE
      prj.conf
      ${CMAKE_CURRENT_SOURCE_DIR}/configuration/${BOARD}/app_with_mcuboot.conf
    )
  endif()
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project("ZBOSS NCP SoC firmware")

target_sources(app PRIVATE
  src/main.c
)
