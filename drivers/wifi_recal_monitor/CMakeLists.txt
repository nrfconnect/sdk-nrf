# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library_named(wifi_recal_monitor)

zephyr_linker_sources(SECTIONS wifi_recal_channel_linker.ld)
zephyr_library_sources(wifi_recal_capture.c)

zephyr_library_sources_ifdef(CONFIG_WIFI_RECAL_DIE_TEMP_MONITOR temperature_monitor.c)

zephyr_include_directories(.)
