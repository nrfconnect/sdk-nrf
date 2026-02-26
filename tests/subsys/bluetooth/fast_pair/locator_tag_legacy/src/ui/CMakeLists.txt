#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE common.c)
target_sources(app PRIVATE platform_dk.c)

zephyr_linker_sources(DATA_SECTIONS request_listener.ld)
