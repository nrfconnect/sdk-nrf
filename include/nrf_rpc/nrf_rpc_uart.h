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

#define NRF_RPC_MAX_FRAME_SIZE	1536
#define NRF_RPC_RX_RINGBUF_SIZE 2048

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

	/* Workqueue for rx*/
	struct k_work_q rx_workq;
} nrf_rpc_uart;

/**
 * @brief Returns nRF RPC UART transport object name for the given devicetree node.
 *
 * @param node_id Devicetree node of the selected UART peripheral (e.g. DT_NODELABEL(uart0)).
 */
#define NRF_RPC_UART_TRANSPORT(node_id) _CONCAT(nrf_rpc_tr_, DT_DEP_ORD(node_id))

#define _NRF_RPC_UART_TRANSPORT_DECLARE(node_id)                                                   \
	extern const struct nrf_rpc_tr NRF_RPC_UART_TRANSPORT(node_id);

DT_FOREACH_STATUS_OKAY(nordic_nrf_uarte, _NRF_RPC_UART_TRANSPORT_DECLARE);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NRF_RPC_UART_H_ */
