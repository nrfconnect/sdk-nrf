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

if (CONFIG_SW_CODEC_G722)
  # libg722 is a plain west project without Zephyr module integration
  get_filename_component(G722_DIR ${ZEPHYR_BASE}/../modules/lib/g722 ABSOLUTE)

  target_sources(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sw_codec_g722.c
    ${G722_DIR}/g722_encode.c
    ${G722_DIR}/g722_decode.c
  )
  target_include_directories(app PRIVATE ${G722_DIR})
endif()
