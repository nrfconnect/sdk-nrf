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

target_compile_definitions(app PRIVATE
  CONFIG_SW_CODEC_G722=1
  CONFIG_SW_CODEC_G722_COMPANY_ID=0
  CONFIG_SW_CODEC_G722_VENDOR_CODEC_ID=1
  CONFIG_AUDIO_SAMPLE_RATE_48000_HZ=1
  CONFIG_AUDIO_SAMPLE_RATE_HZ=48000
  CONFIG_AUDIO_BIT_DEPTH_BITS=16
  CONFIG_AUDIO_BIT_DEPTH_OCTETS=2
  CONFIG_AUDIO_FRAME_DURATION_US=10000
  CONFIG_AUDIO_ENCODE_CHANNELS_MAX=2
  CONFIG_AUDIO_DECODE_CHANNELS_MAX=2
  CONFIG_AUDIO_INPUT_CHANNELS=2
  CONFIG_AUDIO_OUTPUT_CHANNELS=2
  CONFIG_SW_CODEC_SELECT_LOG_LEVEL=3
  CONFIG_BLE_LOG_LEVEL=3
  CONFIG_SERVER_STORE_LOG_LEVEL=3
  CONFIG_CODEC_CAP_COUNT_MAX=5
  CONFIG_BT_AUDIO_PREF_SINK_SAMPLE_RATE_VALUE=8
  CONFIG_BT_AUDIO_PREF_SOURCE_SAMPLE_RATE_VALUE=8
  CONFIG_LC3_BITRATE_MIN=32000
  CONFIG_LC3_BITRATE_MAX=124000
)

if(TEST_MONO_DUPLICATION)
  target_compile_definitions(app PRIVATE CONFIG_MONO_TO_ALL_RECEIVERS=1)
endif()

target_link_libraries(app PRIVATE
  -Wl,--wrap=g722_encoder_new
  -Wl,--wrap=g722_decoder_new
  -Wl,--wrap=g722_encoder_destroy
  -Wl,--wrap=g722_decoder_destroy
  -Wl,--wrap=sample_rate_converter_open
)
