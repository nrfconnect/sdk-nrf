# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library_amend()
zephyr_library_sources_ifdef(CONFIG_REGULATOR_NRF54L regulator_nrf54l.c)
