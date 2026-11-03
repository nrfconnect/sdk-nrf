/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * DECT NR+ RPC uses UART transport between client and server (no IPC).
 * Set nordic,rpc-uart in devicetree (e.g. overlay) to the UART instance.
 */

#if defined(CONFIG_DECT_NR_RPC_UART_TRANSPORT)
#include <nrf_rpc/nrf_rpc_uart.h>
#elif defined(CONFIG_MOCK_DECT_RPC_TRANSPORT)
#include <mock_dect_rpc_transport.h>
#elif defined(CONFIG_MOCK_NRF_RPC_TRANSPORT)
#include <mock_nrf_rpc_transport.h>
#else
#error "DECT RPC: set CONFIG_DECT_NR_RPC_UART_TRANSPORT or MOCK_DECT/MOCK_NRF RPC transport"
#endif
#include <nrf_rpc_cbor.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include "dect_rpc_common.h"

#if defined(CONFIG_DECT_NR_RPC_UART_TRANSPORT)
#define dect_rpc_tr NRF_RPC_UART_TRANSPORT(DT_CHOSEN(nordic_rpc_uart))
#elif defined(CONFIG_MOCK_DECT_RPC_TRANSPORT)
#define dect_rpc_tr mock_dect_rpc_tr
#elif defined(CONFIG_MOCK_NRF_RPC_TRANSPORT)
#define dect_rpc_tr mock_nrf_rpc_tr
#endif

NRF_RPC_GROUP_DEFINE(dect_rpc_group, "dect_rpc", &dect_rpc_tr, NULL, NULL, NULL);
