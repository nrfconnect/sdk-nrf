#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(SB_CONFIG_AUDIO_DFU_EXTERNAL_FLASH)
	add_overlay_dts(nrf5340_audio
		${CMAKE_CURRENT_LIST_DIR}/dfu/conf/overlay-dfu_external_flash.overlay)
	add_overlay_dts(mcuboot
		${CMAKE_CURRENT_LIST_DIR}/dfu/conf/overlay-dfu_external_flash.overlay)

	add_overlay_config(nrf5340_audio
		${CMAKE_CURRENT_LIST_DIR}/dfu/conf/overlay-nrf5340-audio-ext-dfu.conf)
	add_overlay_config(mcuboot
		${CMAKE_CURRENT_LIST_DIR}/dfu/conf/overlay-mcuboot-ext-dfu.conf)

	set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/dfu/conf/pm_dfu_external_flash.yml CACHE INTERNAL "")
	set(mcuboot_PM_STATIC_YML_FILE ${CMAKE_CURRENT_LIST_DIR}/dfu/conf/pm_dfu_external_flash.yml CACHE INTERNAL "")
endif()
