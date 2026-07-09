#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("test_rate_control")

target_sources(app PRIVATE
  src/main.c
  src/api_param_tests.c
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/modules/audio_rate_control/audio_rate_control.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/modules/audio_rate_control
)
