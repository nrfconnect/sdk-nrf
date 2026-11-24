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

  if(NOT g722_west_result STREQUAL "0")
    message(FATAL_ERROR "CONFIG_SW_CODEC_G722 requires the g722 west project")
  endif()

  if(NOT EXISTS "${g722_dir}/g722_encode.c" OR NOT EXISTS "${g722_dir}/g722_decode.c")
    message(FATAL_ERROR "The g722 west project is incomplete: ${g722_dir}")
  endif()

  target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sw_codec_g722.c
    ${g722_dir}/g722_encode.c
    ${g722_dir}/g722_decode.c
  )
  target_include_directories(app PRIVATE ${g722_dir})
endif()
