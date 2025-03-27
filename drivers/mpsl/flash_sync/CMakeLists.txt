#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_NRF_RADIO_SYNC_MPSL flash_sync_mpsl.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_NRF_RADIO_SYNC_RPC_HOST flash_sync_rpc_host.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_NRF_RADIO_SYNC_RPC_CONTROLLER flash_sync_rpc_controller.c)
zephyr_include_directories(${ZEPHYR_BASE}/drivers/flash)
