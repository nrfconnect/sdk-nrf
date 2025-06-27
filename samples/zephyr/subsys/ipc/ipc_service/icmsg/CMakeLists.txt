#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(NOT CONFIG_BOARD_NRF54LV10DK_NRF54LV10A_CPUAPP AND
   NOT CONFIG_BOARD_NRF54LM20DK_NRF54LM20A_CPUAPP)
  message(FATAL_ERROR "${BOARD} is not supported for this sample")
endif()

if(NOT SYSBUILD)
  message(FATAL_ERROR
    " This is a multi-image application that should be built using sysbuild.\n"
    " Add --sysbuild argument to west build command to prepare all the images.")
endif()

project(ipc_service_host)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/samples/subsys/ipc/ipc_service/icmsg/common)

target_sources(app PRIVATE ${ZEPHYR_BASE}/samples/subsys/ipc/ipc_service/icmsg/src/main.c)

include(ExternalProject)
