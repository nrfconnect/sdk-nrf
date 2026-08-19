#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(NOT SYSBUILD)
  message(FATAL_ERROR
    " This is a multi-image application that should be built using sysbuild.\n"
    " Add --sysbuild argument to west build command to prepare all the images."
  )
endif()

project(idle_spim_loopback_on_ppr)

target_sources(app PRIVATE ${ZEPHYR_BASE}/samples/basic/minimal/src/main.c)
