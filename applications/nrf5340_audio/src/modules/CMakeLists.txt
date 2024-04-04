#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/audio_i2s.c
  ${CMAKE_CURRENT_SOURCE_DIR}/audio_usb.c
  ${CMAKE_CURRENT_SOURCE_DIR}/button_handler.c
  ${CMAKE_CURRENT_SOURCE_DIR}/hw_codec.c
  ${CMAKE_CURRENT_SOURCE_DIR}/led.c
  ${CMAKE_CURRENT_SOURCE_DIR}/power_meas.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sd_card.c
)

list(APPEND SRCS ${CMAKE_CURRENT_SOURCE_DIR}/audio_sync_timer_rtc.c)

if (CONFIG_SD_CARD_PLAYBACK)
  list(APPEND SRCS ${CMAKE_CURRENT_SOURCE_DIR}/sd_card_playback.c)
endif()

target_sources(app PRIVATE ${SRCS})
