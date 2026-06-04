# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library_named(vtf_monitoring)

zephyr_linker_sources(SECTIONS vtf_channel_linker.ld)
zephyr_library_sources(vtf_capture.c)

zephyr_include_directories(.)
