# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library_named(vtf_monitoring)

zephyr_linker_sources(SECTIONS vtf_channel_linker.ld)
zephyr_iterable_section(NAME vtf_channel KVMA RAM_REGION GROUP RODATA_REGION)
zephyr_library_sources(vtf_capture.c)

zephyr_library_sources_ifdef(CONFIG_VTF_DIE_TEMP_MONITOR temperature_monitor.c)
zephyr_library_sources_ifdef(CONFIG_VTF_BATTERY_VOLTAGE_MONITOR battery_monitor.c)

zephyr_include_directories(.)
