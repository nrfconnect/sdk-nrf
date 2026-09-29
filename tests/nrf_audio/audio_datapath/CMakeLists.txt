#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_audio_datapath)

target_sources(app PRIVATE
  src/main.c
)

target_compile_definitions(app PRIVATE
  HEADSET=1
  GATEWAY=2
)

target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/audio
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/modules
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/modules/audio_rate_control
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/utils
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/utils/macros
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/src/bluetooth/bt_stream
  ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio/include
  ${ZEPHYR_NRF_MODULE_DIR}/include
)
