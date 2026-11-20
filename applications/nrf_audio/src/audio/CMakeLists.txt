#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/audio_system.c
  ${CMAKE_CURRENT_SOURCE_DIR}/audio_datapath.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sw_codec_select.c
  ${CMAKE_CURRENT_SOURCE_DIR}/le_audio_rx.c
)

if(CONFIG_SW_CODEC_G722)
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

  if(NOT G722_WEST_RESULT STREQUAL "0")
    message(FATAL_ERROR "CONFIG_SW_CODEC_G722 requires the g722 west project")
  endif()

  if(NOT EXISTS "${G722_DIR}/g722_encode.c" OR NOT EXISTS "${G722_DIR}/g722_decode.c")
    message(FATAL_ERROR "The g722 west project is incomplete: ${G722_DIR}")
  endif()

  target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sw_codec_g722.c
    ${G722_DIR}/g722_encode.c
    ${G722_DIR}/g722_decode.c
  )
  target_include_directories(app PRIVATE ${G722_DIR})
endif()
