#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_hf_audio_clock)

target_sources(app PRIVATE
  src/main.c
  src/hf_audio_clock_tests.c
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/modules/hf_audio_clock.c
)

target_compile_definitions(app PRIVATE
  NRFX_DECLARE_ONLY
  CONFIG_MODULE_RATE_CONTROL_LOG_LEVEL=0
)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/modules
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/modules/audio_rate_control
)
