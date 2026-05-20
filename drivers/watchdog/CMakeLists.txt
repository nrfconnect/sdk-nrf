#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library_amend()
zephyr_library_sources_ifdef(CONFIG_NRF_GSWDT nrf_gswdt.c)
