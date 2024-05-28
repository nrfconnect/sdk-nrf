#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/audio_i2s.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/audio_usb.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/button_handler.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/audio_sync_timer.c)

target_sources_ifdef(CONFIG_NRF5340_AUDIO_CS47L63_DRIVER app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/hw_codec.c)
target_sources_ifdef(CONFIG_NRF5340_AUDIO_POWER_MEASUREMENT app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/power_meas.c)
target_sources_ifdef(CONFIG_NRF5340_AUDIO_SD_CARD_MODULE app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/sd_card.c)
target_sources_ifdef(CONFIG_SD_CARD_PLAYBACK app PRIVATE
                     ${CMAKE_CURRENT_SOURCE_DIR}/sd_card_playback.c)
