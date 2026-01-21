#
# Copyright (c) 2025 Nordic Semiconductor ASA
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

project(uart_latency)

if(CONFIG_UART_INTERRUPT_DRIVEN)
  list(APPEND app_sources src/int_driven.c)
elseif(CONFIG_UART_ASYNC_API)
  list(APPEND app_sources src/async.c)
else()
  list(APPEND app_sources src/polling.c)
endif()

target_sources(app PRIVATE src/common.c ${app_sources})
