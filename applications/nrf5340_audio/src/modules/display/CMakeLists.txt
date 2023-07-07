#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_NRF5340_AUDIO_DK_DISPLAY)
	target_sources(app PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/nrf5340_audio_dk_display.c
		${CMAKE_CURRENT_SOURCE_DIR}/tab_menu.c
	)
	if (CONFIG_NRF5340_AUDIO_DK_DISPLAY_TAB_LOG)
		target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tab_log.c)
	endif()
endif()
