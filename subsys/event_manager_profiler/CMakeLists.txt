#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
zephyr_sources_ifdef(CONFIG_EVENT_MANAGER_PROFILER event_manager_profiler.c)
