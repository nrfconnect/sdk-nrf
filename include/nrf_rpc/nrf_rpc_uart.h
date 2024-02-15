/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_UART_H_
#define NRF_RPC_UART_H_

#include <zephyr/device.h>
#include <nrf_rpc.h>
#include <nrf_rpc_tr.h>

typedef struct nrf_rpc_uart {
    const struct device *uart;
	nrf_rpc_tr_receive_handler_t receive_callback;
	const struct nrf_rpc_tr *transport;
} nrf_rpc_uart;

extern const struct nrf_rpc_tr_api nrf_rpc_uart_service_api;

#define NRF_RPC_UART_TRANSPORT(_name, _uart)                        \
	static struct nrf_rpc_uart _name##_instance = {                       \
	       .uart = _uart,               \
		   .receive_callback = NULL,                                    \
		   .transport = NULL,                   \
	};                                                                   \
	const struct nrf_rpc_tr _name = {                                    \
		.api = &nrf_rpc_uart_service_api,                             \
		.ctx = &_name##_instance                                     \
	}

#endif