/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NRF_RPC_UART_H_
#define NRF_RPC_UART_H_

#include <device.h>

#include <nrf_rpc.h>
#include <nrf_rpc_tr.h>
#include <zephyr/sys/ring_buffer.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_rpc_uart nRF RPC UART Service transport
 * @brief nRF RPC UART Service transport.
 *
 * @{
 */

/*  nRF UART Service transport API strucure. It contains all
 *  necessary functions required by the nRF RPC library.
 */
extern const struct nrf_rpc_tr_api nrf_rpc_uart_api;

/* We have to use an additional header because the nRF RPC header
 * does not encode packet length information.
 */
struct nrf_rpc_uart_header {
	char start[4];		/* spells U A R T */
	uint16_t len;
	uint8_t crc;		/* CRC of whole frame */
	uint8_t idx;		/* Current index, used when building header */
};

/** @brief nRF RPC UART Service transport instance. */
struct nrf_rpc_uart {
	const struct device *uart;

	/* RPC data received callback. Should be called on completed packet. */
	nrf_rpc_tr_receive_handler_t receive_cb;

	/** User context. */
	void *context;

	/** Indicates if transport is already initialized. */
	bool used;

	struct nrf_rpc_uart_header *header;

	/* ring buffer: stores all received uart data */
	struct ring_buf *ringbuf;

	/* packet buffer: stores only the packet to be sent to nRF RPC */
	char *packet;

	/* Dispatches callbacks into nRF RPC */
	struct k_work work;

	/* Used to access the context from the work item */
	const struct nrf_rpc_tr *transport;
};

/** @brief Extern nRF RPC UART Service transport declaration.
 *
 * Can be used in header files. It is useful when several nRF RPC groups are
 * defined amongst different source files but share the same transport instance.
 *
 * @param[in] _name Name of the nRF RPC transport.
 */
#define NRF_RPC_UART_TRANSPORT_DECLARE(_name) \
	extern const struct nrf_rpc_tr _name

/** @brief Defines the nRF UART Transport instance.
 *
 * It creates the nRF RPC UART Service transport instance. The @p _uart parameter defines
 * the destination remote CPU. A single instance of this transport can be shared between
 * several nRF RPC groups.
 *
 * Example:
 *
 *  * Two groups share the same UART instance:
 *
 *      NRF_RPC_UART_TRANSPORT(nrf_rpc_1, DEVICE_DT_GET(DT_NODELABEL(uart0)));
 *
 *      NRF_RPC_GROUP_DEFINE(group_1, "Group_1", &nrf_rpc_1, NULL, NULL, NULL);
 *      NRF_RPC_GROUP_DEFINE(group_2, "Group_2", &nrf_rpc_1, NULL, NULL, NULL);
 *
 *  * Each group uses a different UART instance:
 *
 *      NRF_RPC_UART_TRANSPORT(nrf_rpc_1, DEVICE_DT_GET(DT_NODELABEL(uart0)));
 *      NRF_RPC_UART_TRANSPORT(nrf_rpc_2, DEVICE_DT_GET(DT_NODELABEL(uart1)));
 *
 *      NRF_RPC_GROUP_DEFINE(group_1, "Group_1", &nrf_rpc_1, NULL, NULL, NULL);
 *      NRF_RPC_GROUP_DEFINE(group_2, "Group_2", &nrf_rpc_2, NULL, NULL, NULL);
 *
 * @param[in] _name nRF RPC UART Service transport instance name.
 * @param[in] _uart The instance used for the UART Service to transfer data between CPUs.
 */
#define NRF_RPC_UART_TRANSPORT(_name, _uart)				\
	static uint8_t _name##_packet[CONFIG_NRF_RPC_UART_BUF_SIZE] = {0}; \
	RING_BUF_DECLARE(_name##_ringbuf, CONFIG_NRF_RPC_UART_BUF_SIZE); \
	static struct nrf_rpc_uart_header _name##_header_state;		\
	                                                                \
	NRF_RPC_UART_TRANSPORT_DECLARE(_name);				\
	static struct nrf_rpc_uart _name##_instance = {                 \
		.uart = _uart,                                          \
		.ringbuf = &_name##_ringbuf,				\
		.packet = _name##_packet,				\
		.header = &_name##_header_state,			\
		.transport = &_name,					\
	};                                                              \
									\
	const struct nrf_rpc_tr _name = {                               \
		.api = &nrf_rpc_uart_api,				\
		.ctx = &_name##_instance                                \
	};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* NRF_RPC_UART_H_ */
