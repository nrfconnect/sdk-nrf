#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
zephyr_sources_ifdef(CONFIG_APP_EVENT_MANAGER_PROFILER_TRACER app_event_manager_profiler_tracer.c)
zephyr_linker_sources(SECTIONS em_pt.ld)
zephyr_iterable_section(NAME nrf_profiler_info KVMA RAM_REGION GROUP RODATA_REGION)
