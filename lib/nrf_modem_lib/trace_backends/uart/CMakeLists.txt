#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART)
# Feature toggle for experimental thread based trace processing
zephyr_library_sources_ifndef(
  CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PROCESSING
  uart_sync.c
)
zephyr_library_sources_ifdef(
  CONFIG_NRF_MODEM_LIB_TRACE_THREAD_PROCESSING
  uart.c
)
endif()
