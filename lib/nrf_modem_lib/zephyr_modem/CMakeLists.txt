# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

zephyr_library()
zephyr_library_sources(nrf_modem_lib_zephyr_modem.c)

# nrfxlib AT headers need to be globally visible (at_monitor, lte_lc, etc.)
zephyr_include_directories(${ZEPHYR_NRFXLIB_MODULE_DIR}/nrf_modem/include)
