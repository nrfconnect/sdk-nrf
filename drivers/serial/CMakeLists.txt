#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_amend()
zephyr_library_sources_ifdef(CONFIG_NRF_SW_LPUART uart_nrf_sw_lpuart.c)
zephyr_library_sources_ifdef(CONFIG_IPC_UART uart_ipc.c)
