#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_sources_ifdef(CONFIG_DM_MODULE dm.c)
zephyr_library_sources_ifdef(CONFIG_DM_MODULE timeslot_queue.c)
zephyr_library_sources_ifdef(CONFIG_DM_MODULE time.c)
zephyr_library_sources_ifdef(CONFIG_DM_MODULE dm_io.c)
