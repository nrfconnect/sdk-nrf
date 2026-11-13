#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_sources_ifdef(
  CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_NRFX
  uart_nrfx.c
)
