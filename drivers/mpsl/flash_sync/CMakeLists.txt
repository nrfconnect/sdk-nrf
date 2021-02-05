#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(flash_sync_mpsl.c)
zephyr_include_directories(${ZEPHYR_BASE}/drivers/flash)
