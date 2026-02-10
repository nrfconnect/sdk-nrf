#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE
  audio_i2s.c
  button_handler.c
  led.c
  audio_sync_timer.c
  audio_clock.c
)

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/subsys/usb/device_next/class
)

target_sources_ifdef(CONFIG_AUDIO_SOURCE_USB app PRIVATE
  audio_usb.c
  audio_usb_init.c
)

target_sources_ifdef(CONFIG_NRF5340_AUDIO_CS47L63_DRIVER app PRIVATE
  hw_codec.c
)

target_sources_ifdef(CONFIG_NRF5340_AUDIO_POWER_MEASUREMENT app PRIVATE
  power_meas.c
)

target_sources_ifdef(CONFIG_NRF5340_AUDIO_SD_CARD_MODULE app PRIVATE
  sd_card.c
)

target_sources_ifdef(CONFIG_NRF5340_AUDIO_SD_CARD_LC3_FILE app PRIVATE
  lc3_file.c
)

target_sources_ifdef(CONFIG_SD_CARD_PLAYBACK app PRIVATE
  sd_card_playback.c
)

target_sources_ifdef(CONFIG_NRF5340_AUDIO_SD_CARD_LC3_STREAMER app PRIVATE
  lc3_streamer.c
)
