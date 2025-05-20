#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_DM_MODULE AND (NOT CONFIG_DM_MODULE_RPC_CLIENT))
	zephyr_library_sources(dm.c)
	zephyr_library_sources(timeslot_queue.c)
	zephyr_library_sources(time.c)
	zephyr_library_sources(dm_io.c)
endif()

add_subdirectory_ifdef(CONFIG_DM_MODULE_RPC rpc)
