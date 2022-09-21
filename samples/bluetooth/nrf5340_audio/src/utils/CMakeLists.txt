#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/board_version.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/channel_assignment.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/contin_array.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/data_fifo.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/error_handler.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/pcm_stream_channel_modifier.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/tone.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/uicr.c
		   ${CMAKE_CURRENT_SOURCE_DIR}/pcm_mix.c
)
