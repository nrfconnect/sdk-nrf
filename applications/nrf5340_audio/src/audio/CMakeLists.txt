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

if (CONFIG_BT_BAP_BROADCAST_SINK)
	target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/streamctrl_broadcast_sink.c)
endif()

if (CONFIG_BT_BAP_BROADCAST_SOURCE)
	target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/streamctrl_broadcast_source.c)
endif()

if (CONFIG_BT_BAP_UNICAST_CLIENT)
	target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/streamctrl_unicast_client.c)
endif()

if (CONFIG_BT_BAP_UNICAST_SERVER)
	target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/streamctrl_unicast_server.c)
endif()
