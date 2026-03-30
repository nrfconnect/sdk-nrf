#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(nrf_lcs.c)
zephyr_library_sources_ifdef(CONFIG_SECURE_BOOT_STORAGE nrf_lcs_mem_bl_storage.c)
zephyr_library_sources_ifndef(CONFIG_PARTITION_MANAGER_ENABLED nrf_lcs_mem_otp.c)
