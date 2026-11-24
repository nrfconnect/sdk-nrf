#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_sw_codec_g722)

set(audio_app ${ZEPHYR_NRF_MODULE_DIR}/applications/nrf_audio)

if(NOT DEFINED g722_dir)
  execute_process(
    COMMAND ${WEST} list g722 -f "{abspath}"
    WORKING_DIRECTORY ${WEST_TOPDIR}
    OUTPUT_VARIABLE g722_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE g722_west_result
  )
else()
  set(g722_west_result 0)
endif()

if(NOT g722_west_result STREQUAL "0" OR NOT EXISTS "${g722_dir}/g722_encode.c"
  OR NOT EXISTS "${g722_dir}/g722_decode.c"
)
  message(WARNING "G.722 library unavailable; codec tests will be skipped")
  set(g722_available n)
else()
  set(g722_available y)
endif()

target_sources(app PRIVATE
  ${audio_app}/src/audio/sw_codec_select.c
  ${audio_app}/src/audio/sw_codec_g722.c
)

target_include_directories(app PRIVATE
  ${audio_app}/src/audio
  ${audio_app}/src/utils
  ${audio_app}/src/utils/macros
)

if(g722_available)
  target_sources(app PRIVATE
    src/main.c
    ${g722_dir}/g722_encode.c
    ${g722_dir}/g722_decode.c
  )
  target_include_directories(app PRIVATE ${g722_dir})
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
