#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(.)
target_sources(app PRIVATE desh_at_cmng.c)
target_sources(app PRIVATE desh_native_tls.c)
