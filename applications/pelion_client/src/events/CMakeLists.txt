#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE
	       ${CMAKE_CURRENT_SOURCE_DIR}/net_event.c
	       ${CMAKE_CURRENT_SOURCE_DIR}/pelion_event.c
)

target_sources_ifdef(CONFIG_PELION_CLIENT_KEEP_ALIVE_EVENTS
		     app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/keep_alive_event.c)
