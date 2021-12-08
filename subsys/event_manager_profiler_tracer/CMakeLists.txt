#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
zephyr_sources_ifdef(CONFIG_EVENT_MANAGER_PROFILER_TRACER event_manager_profiler_tracer.c)
