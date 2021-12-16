#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_sources_ifdef(CONFIG_NRF_DM dm.c)
zephyr_library_sources_ifdef(CONFIG_NRF_DM timeslot_queue.c)
zephyr_library_sources_ifdef(CONFIG_NRF_DM time.c)
zephyr_library_sources_ifdef(CONFIG_NRF_DM dm_io.c)
