#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_bt_le_audio_tx)

target_sources(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/bluetooth/bt_stream/bt_le_audio_tx/bt_le_audio_tx.c
  ${ZEPHYR_NRF_MODULE_DIR}/tests/nrf5340_audio/fakes/hci_vs_sdc/hci_vs_sdc_fake.c
  src/main.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/tests/nrf5340_audio/fakes
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/bluetooth/bt_stream/bt_le_audio_tx
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/bluetooth/bt_stream
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/modules
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/include
  ${ZEPHYR_NRF_MODULE_DIR}/include
)

target_compile_options(app PRIVATE
  -DHEADSET=1
  -DGATEWAY=2
)
