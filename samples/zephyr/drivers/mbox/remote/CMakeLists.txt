#
# Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
# Copyright 2023-2025 NXP
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if(CONFIG_BOARD_NRF54LM20PDK_NRF54LM20A_CPUFLPR OR
   CONFIG_BOARD_NRF54LV10DK_NRF54LV10A_CPUFLPR)
  message(STATUS "${BOARD}${BOARD_QUALIFIERS} compile as remote in this sample")
else()
  message(FATAL_ERROR "${BOARD}${BOARD_QUALIFIERS} is not supported for this sample")
endif()

project(mbox_ipc_remote)

target_sources(app PRIVATE ${ZEPHYR_BASE}/samples/drivers/mbox/remote/src/main.c)
