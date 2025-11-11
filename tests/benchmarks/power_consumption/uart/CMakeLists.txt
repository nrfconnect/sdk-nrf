#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
zephyr_include_directories(include)

project(uart_async)

if(CONFIG_UART_INTERRUPT_DRIVEN)
  list(APPEND app_sources src/int_driver_test.c)
elseif(CONFIG_UART_ASYNC_API)
  list(APPEND app_sources src/async_driver_test.c)
else()
  list(APPEND app_sources src/poll_driver_test.c)
endif()

target_sources(app PRIVATE ./../common/main.c ${app_sources})
