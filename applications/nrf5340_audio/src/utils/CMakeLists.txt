#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/device_location.c
  ${CMAKE_CURRENT_SOURCE_DIR}/error_handler.c
  ${CMAKE_CURRENT_SOURCE_DIR}/uicr.c
  ${CMAKE_CURRENT_SOURCE_DIR}/peripherals.c
)

target_sources_ifdef(CONFIG_BOARD_NRF5340_AUDIO_DK_NRF5340_CPUAPP app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/nrf5340_audio_dk_version.c
)
