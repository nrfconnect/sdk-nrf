#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Feature toggle for experimental thread based trace processing
zephyr_library_sources_ifndef(
  CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_SYNC
  uart.c
)
zephyr_library_sources_ifdef(
  CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_SYNC
  uart_sync.c
)
