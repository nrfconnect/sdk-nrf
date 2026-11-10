#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_sw_codec_g722)

set(AUDIO_APP ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio)
set(G722_LIB ${ZEPHYR_BASE}/../modules/lib/g722)

target_sources(app PRIVATE
  src/main.c
  src/signaling.c
  ${AUDIO_APP}/src/audio/sw_codec_select.c
  ${AUDIO_APP}/src/audio/sw_codec_g722.c
  ${AUDIO_APP}/src/bluetooth/bt_stream/le_audio.c
  ${AUDIO_APP}/src/bluetooth/bt_stream/unicast/server_store.c
  ${G722_LIB}/g722_encode.c
  ${G722_LIB}/g722_decode.c
)

target_include_directories(app PRIVATE
  ${AUDIO_APP}/src/audio
  ${AUDIO_APP}/src/utils
  ${AUDIO_APP}/src/utils/macros
  ${AUDIO_APP}/src/bluetooth/bt_stream
  ${AUDIO_APP}/src/bluetooth/bt_stream/unicast
  ${G722_LIB}
)

target_link_libraries(app PRIVATE
  -Wl,--wrap=g722_encoder_new
  -Wl,--wrap=g722_decoder_new
  -Wl,--wrap=g722_encoder_destroy
  -Wl,--wrap=g722_decoder_destroy
  -Wl,--wrap=sample_rate_converter_open
)
