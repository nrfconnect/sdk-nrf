/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "rpc_utils_group.h"

#if CONFIG_NRF_RPC_IPC_SERVICE
#include <nrf_rpc/nrf_rpc_ipc.h>
#elif CONFIG_NRF_RPC_UART_TRANSPORT
#include <nrf_rpc/nrf_rpc_uart.h>
#elif CONFIG_MOCK_NRF_RPC_TRANSPORT
#include <mock_nrf_rpc_transport.h>
#endif
#include <nrf_rpc_cbor.h>

#include <zephyr/device.h>

#ifdef CONFIG_NRF_RPC_IPC_SERVICE
NRF_RPC_IPC_TRANSPORT(rpc_utils_rpc_tr, DEVICE_DT_GET(DT_NODELABEL(ipc0)), "rpc_utils_rpc_ept");
#elif defined(CONFIG_NRF_RPC_UART_TRANSPORT)
#define rpc_utils_rpc_tr NRF_RPC_UART_TRANSPORT(DT_CHOSEN(nordic_rpc_uart))
#elif defined(CONFIG_MOCK_NRF_RPC_TRANSPORT)
#define rpc_utils_rpc_tr mock_nrf_rpc_tr
#endif
NRF_RPC_GROUP_DEFINE(rpc_utils_group, "rpc_utils", &rpc_utils_rpc_tr, NULL, NULL, NULL);
