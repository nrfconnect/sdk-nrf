#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_bt_le_audio_tx)

target_sources(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/bluetooth/bt_stream/bt_le_audio_tx/bt_le_audio_tx.c
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/bluetooth/bt_stream/le_audio.c
  ${ZEPHYR_NRF_MODULE_DIR}/tests/nrf_audio/fakes/hci_vs_sdc/hci_vs_sdc_fake.c
  ${ZEPHYR_NRF_MODULE_DIR}/tests/nrf_audio/fakes/zbus_fakes/zbus_fakes.c
  ${ZEPHYR_NRF_MODULE_DIR}/tests/nrf_audio/fakes/application/application.c
  src/main.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/tests/nrf_audio/fakes
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/bluetooth/bt_stream/bt_le_audio_tx
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/bluetooth/bt_stream
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/modules
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/include
  ${ZEPHYR_NRF_MODULE_DIR}/tests/nrf_audio/fakes/
  ${ZEPHYR_NRF_MODULE_DIR}/include
)
