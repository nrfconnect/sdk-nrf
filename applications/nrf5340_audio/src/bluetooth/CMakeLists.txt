#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_subdirectory(bt_management)
add_subdirectory(bt_renderer)
add_subdirectory(bt_content_control)

zephyr_library_include_directories(
    bt_management
    bt_renderer
    bt_content_control
)

if (CONFIG_TRANSPORT_CIS)
	# HEADSET
	if (CONFIG_AUDIO_DEV EQUAL 1)
		target_sources(app PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/le_audio_cis_headset.c)
	# GATEWAY
	elseif (CONFIG_AUDIO_DEV EQUAL 2)
		target_sources(app PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/le_audio_cis_gateway.c)
	else()
		message(FATAL_ERROR "GATEWAY or HEADSET device must be chosen")
	endif()
elseif (CONFIG_TRANSPORT_BIS)
	# HEADSET
	if (CONFIG_AUDIO_DEV EQUAL 1)
		target_sources(app PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/le_audio_bis_headset.c)
	# GATEWAY
	elseif (CONFIG_AUDIO_DEV EQUAL 2)
		target_sources(app PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/le_audio_bis_gateway.c)
	else()
		message(FATAL_ERROR "GATEWAY or HEADSET device must be chosen")
	endif()
else()
	message(FATAL_ERROR "CIS or BIS mode must be chosen")
endif()
