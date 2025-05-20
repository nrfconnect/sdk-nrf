#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(NONE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

if (DEFINED CONFIG_SOC_NRF9160)
  set(family_arg NRF91)
else()
  set(family_arg NRF53)
endif()


if("${CONFIG_HUK_TEST_BOARD_SNR}" STREQUAL "")
  message(WARNING "If it is to be run on target, this test requires setting "
      "CONFIG_HUK_TEST_BOARD_SNR")
else()
  add_custom_command(TARGET app
    POST_BUILD
    COMMAND
    ${PYTHON_EXECUTABLE}
    ${CMAKE_CURRENT_LIST_DIR}/write_kmu.py
    --family ${family_arg}
    --snr ${CONFIG_HUK_TEST_BOARD_SNR}
    )
endif()
