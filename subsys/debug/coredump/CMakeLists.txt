#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_sources_ifdef(CONFIG_DEBUG_COREDUMP_BACKEND_NRF_FLASH_PARTITION coredump_backend_nrf_flash_partition.c)
