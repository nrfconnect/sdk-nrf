#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_library()
zephyr_library_sources(flash_sync_mpsl.c)
zephyr_include_directories(${ZEPHYR_BASE}/drivers/flash)
