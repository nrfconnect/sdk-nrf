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
#include <zephyr/sys/ring_buffer.h>

#define NRF_RPC_MAX_FRAME_SIZE	1536
#define NRF_RPC_RX_RINGBUF_SIZE 64

typedef enum {
	hdlc_state_unsync,
	hdlc_state_frame_start,
	hdlc_state_frame_found,
	hdlc_state_escape,
} hdlc_state_t;

typedef struct nrf_rpc_uart {
	const struct device *uart;
	nrf_rpc_tr_receive_handler_t receive_callback;
	void *receive_ctx;
	const struct nrf_rpc_tr *transport;

	/* RX buffer populated by UART ISR */
	uint8_t rx_buffer[NRF_RPC_RX_RINGBUF_SIZE];
	struct ring_buf rx_ringbuf;

	/* ISR function callback worker */
	struct k_work cb_work;

	/* HDLC frame parsing state */
	hdlc_state_t hdlc_state;
	uint8_t frame_buffer[NRF_RPC_MAX_FRAME_SIZE];
	size_t frame_len;

	/* UART send semaphore */
	struct k_sem uart_tx_sem;
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
