#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_sw_codec_g722)

set(AUDIO_APP ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio)

if(NOT DEFINED G722_DIR)
  execute_process(
    COMMAND ${WEST} list g722 -f "{abspath}"
    WORKING_DIRECTORY ${WEST_TOPDIR}
    OUTPUT_VARIABLE G722_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE G722_WEST_RESULT
  )
else()
  set(G722_WEST_RESULT 0)
endif()

if(NOT G722_WEST_RESULT STREQUAL "0" OR NOT EXISTS "${G722_DIR}/g722_encode.c"
   OR NOT EXISTS "${G722_DIR}/g722_decode.c"
)
  message(WARNING "G.722 library unavailable; codec tests will be skipped")
  set(g722_available FALSE)
else()
  set(g722_available TRUE)
endif()

target_sources(app PRIVATE
  ${AUDIO_APP}/src/audio/sw_codec_select.c
  ${AUDIO_APP}/src/audio/sw_codec_g722.c
)

target_include_directories(app PRIVATE
  ${AUDIO_APP}/src/audio
  ${AUDIO_APP}/src/utils
  ${AUDIO_APP}/src/utils/macros
)

if(g722_available)
  target_sources(app PRIVATE
    src/main.c
    ${G722_DIR}/g722_encode.c
    ${G722_DIR}/g722_decode.c
  )
  target_include_directories(app PRIVATE ${G722_DIR})
  target_link_libraries(app PRIVATE
    -Wl,--wrap=g722_encoder_new
    -Wl,--wrap=g722_decoder_new
    -Wl,--wrap=g722_encoder_destroy
    -Wl,--wrap=g722_decoder_destroy
    -Wl,--wrap=sample_rate_converter_open
  )
else()
  target_sources(app PRIVATE src/unavailable.c)
endif()
