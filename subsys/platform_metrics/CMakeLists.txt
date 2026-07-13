# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library_named(platform_metrics)

zephyr_linker_sources(SECTIONS platform_metrics_channel_linker.ld)
zephyr_library_sources(platform_metrics_capture.c)

zephyr_library_sources_ifdef(CONFIG_PLATFORM_METRICS_DIE_TEMP_MONITOR temperature_monitor.c)

zephyr_include_directories(.)
