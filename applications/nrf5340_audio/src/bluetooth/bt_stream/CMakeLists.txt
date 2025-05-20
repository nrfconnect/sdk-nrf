#
# Copyright (c) 2023 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_include_directories(
    broadcast
    unicast
    bt_le_audio_tx
)

add_subdirectory(bt_le_audio_tx)

target_sources(app PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/le_audio.c)

if (CONFIG_BT_BAP_BROADCAST_SINK)
	target_sources(app PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/broadcast/broadcast_sink.c)
endif()

if (CONFIG_BT_BAP_BROADCAST_SOURCE)
	target_sources(app PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/broadcast/broadcast_source.c)
endif()

if (CONFIG_BT_BAP_UNICAST_CLIENT)
	target_sources(app PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/unicast/unicast_client.c)
endif()

if (CONFIG_BT_BAP_UNICAST_SERVER)
	target_sources(app PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/unicast/unicast_server.c)
endif()
