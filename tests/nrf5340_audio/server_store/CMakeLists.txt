#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_server_store)

target_sources(app
  PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/bluetooth/bt_stream/unicast/server_store.c
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/bluetooth/bt_stream/le_audio.c
  main.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/utils/macros/
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/bluetooth/bt_stream/unicast/
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf5340_audio/src/bluetooth/bt_stream/
)

target_compile_options(app
  PRIVATE
  -DCONFIG_CODEC_CAP_COUNT_MAX=5
  -DCONFIG_BT_AUDIO_PREF_SINK_SAMPLE_RATE_VALUE=8
  -DCONFIG_BT_AUDIO_PREF_SOURCE_SAMPLE_RATE_VALUE=8
  -DCONFIG_AUDIO_SAMPLE_RATE_48000_HZ
  -DCONFIG_SERVER_STORE_LOG_LEVEL=4
  -DCONFIG_BLE_LOG_LEVEL=4
  -DCONFIG_LC3_BITRATE_MIN=32000
  -DCONFIG_LC3_BITRATE_MAX=124000
  -DCONFIG_AUDIO_FRAME_DURATION_US=10000
  )
