/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_UART_H_
#define NRF_RPC_UART_H_

#include <nrf_rpc.h>
#include <nrf_rpc_tr.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/ring_buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_rpc_uart nRF RPC UART transport
 * @brief nRF RPC UART transport.
 *
 * @{
 */

/**
 * @brief Returns nRF RPC UART transport object name for the given devicetree node.
 *
 * @param node_id Devicetree node of the selected UART peripheral (e.g. DT_NODELABEL(uart0)).
 */
#define NRF_RPC_UART_TRANSPORT(node_id) _CONCAT(nrf_rpc_tr_, DT_DEP_ORD(node_id))

/**
 * @}
 */

#define _NRF_RPC_UART_TRANSPORT_DECLARE(node_id)                                                   \
	extern const struct nrf_rpc_tr NRF_RPC_UART_TRANSPORT(node_id);

DT_FOREACH_STATUS_OKAY(nordic_nrf_uarte, _NRF_RPC_UART_TRANSPORT_DECLARE);

#ifdef __cplusplus
}
#endif

#endif /* NRF_RPC_UART_H_ */
