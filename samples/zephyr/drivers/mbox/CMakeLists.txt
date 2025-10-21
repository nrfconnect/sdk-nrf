#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

set(REMOTE_ZEPHYR_DIR ${CMAKE_CURRENT_BINARY_DIR}/../remote/zephyr)

if(CONFIG_BOARD_NRF54LV10DK_NRF54LV10A_CPUAPP OR
   CONFIG_BOARD_NRF54LV10DK_NRF54LV10A_CPUAPP_NS)
  message(STATUS "${BOARD}${BOARD_QUALIFIERS} compile as Main in this sample")
else()
  message(FATAL_ERROR "${BOARD}${BOARD_QUALIFIERS} is not supported for this sample")
endif()

if(NOT SYSBUILD)
  message(FATAL_ERROR
    " This is a multi-image application that should be built using sysbuild.\n"
    " Add --sysbuild argument to west build command to prepare all the images.")
endif()

project(mbox_ipc)

enable_language(C ASM)

target_sources(app PRIVATE ${ZEPHYR_BASE}/samples/drivers/mbox/src/main.c)
