#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

target_sources_ifndef(CONFIG_NRF_SW_LPUART_INT_DRIVEN app PRIVATE
  src/common.c
  src/main.c
)
target_sources_ifdef(CONFIG_NRF_SW_LPUART_INT_DRIVEN app PRIVATE
  src/common.c
  src/int_driven.c
)
