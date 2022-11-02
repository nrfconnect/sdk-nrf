#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/audio_i2s.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/audio_usb.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/button_handler.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/hw_codec.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/led.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/power_meas.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/sd_card.c
)

if (CONFIG_AUDIO_DFU_ENABLE)
	target_sources(app PRIVATE
			    ${CMAKE_CURRENT_SOURCE_DIR}/dfu_entry.c
	)
endif()
