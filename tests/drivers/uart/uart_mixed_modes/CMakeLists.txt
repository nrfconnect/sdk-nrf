#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(uart_mode_to_mode)

if(CONFIG_ASYNC_AND_INT)
  target_sources(app PRIVATE src/common.c src/async_int.c)
elseif(CONFIG_ASYNC_AND_POLL)
  target_sources(app PRIVATE src/common.c src/async_poll.c)
else()
  target_sources(app PRIVATE src/common.c src/int_poll.c)
endif()
