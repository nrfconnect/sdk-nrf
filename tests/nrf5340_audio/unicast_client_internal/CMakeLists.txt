#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_unicast_client_internal)

target_sources(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/bluetooth/bt_stream/unicast/unicast_client_internal.c
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/bluetooth/bt_stream/le_audio.c
  ${ZEPHYR_BASE}/subsys/bluetooth/common/addr.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/codec.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/audio.c
  main.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/utils/macros/
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/bluetooth/bt_stream/unicast/
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/bluetooth/bt_stream/
  ${ZEPHYR_NRF_MODULE_DIR}/tests/nrf5340_audio/fakes/
)

target_compile_options(app
  PRIVATE
  )
