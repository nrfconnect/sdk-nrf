/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrf_rpc.h>
#include <nrf_rpc_tr.h>
#include <nrf_rpc_errno.h>
#include <nrf_rpc/nrf_rpc_uart.h>

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/sys/__assert.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(nrf_rpc_uart, CONFIG_NRF_RPC_TR_LOG_LEVEL);

/* From nrf_rpc_ipc.c */
#define DUMP_LIMITED_DBG(memory, len, text)				       \
do {									       \
	if ((len) > 32) {						       \
		LOG_HEXDUMP_DBG(memory, 32, text " (truncated)");	       \
	} else {							       \
		LOG_HEXDUMP_DBG(memory, (len), text);			       \
	}								       \
} while (0)

static inline void cleanup_state(struct nrf_rpc_uart *config)
{
	LOG_DBG("");
	memset(config->header, 0, sizeof(struct nrf_rpc_uart_header));
}

static void process_ringbuf(struct nrf_rpc_uart *uart_config);

static void rpc_tr_uart_handler(struct k_work *item)
{
	LOG_DBG("");

	struct nrf_rpc_uart *uart_config =
		CONTAINER_OF(item, struct nrf_rpc_uart, work);
	LOG_DBG("work %p", &uart_config->work);

	struct nrf_rpc_uart_header *header = uart_config->header;
	const struct nrf_rpc_tr *transport = uart_config->transport;

	__ASSERT_NO_MSG(uart_config->receive_cb);
	__ASSERT_NO_MSG(uart_config->used);

	ring_buf_get(uart_config->ringbuf, uart_config->packet, header->len);
	LOG_HEXDUMP_DBG(uart_config->packet, header->len, "packet");

	LOG_DBG("calling rx cb");
	/* We memcpy the data out because the packet might reside on the ringbuf
	 * boundary, and nrf-rpc can't handle that, it expects a single linear
	 * array.
	 */
	uart_config->receive_cb(transport,
				uart_config->packet,
				header->len,
				uart_config->context);
	LOG_DBG("rx cb returned");
	cleanup_state(uart_config);

	/* Re-trigger processing in case we have another packet pending. */
	process_ringbuf(uart_config);
}

/* False if header is invalid or incomplete
 * True if header complete and valid
 */
static bool build_header(struct nrf_rpc_uart *uart_config)
{
	struct nrf_rpc_uart_header *header = uart_config->header;

	__ASSERT_NO_MSG(header != NULL);

	if (header->idx > 6) {
		/* Header is complete, the current byte doesn't belong to it */
		return true;
	}

	uint8_t byte;

	if (ring_buf_get(uart_config->ringbuf, &byte, 1) != 1) {
		return false;
	}

	if (header->idx < 4) {
		if (byte != "UART"[header->idx]) {
			return false;
		}
		/* If the rx char matches its required value, header->idx will
		 * be incremented and parsing will continue in the next call.
		 * Else, we cleanup the state and return.
		 */
	} else if (header->idx == 3) {
		/* Don't trigger a memset for each rx'd byte (that doesn't match
		 * the header).
		 */
		cleanup_state(uart_config);
	}

	switch (header->idx) {
	case 4:
		header->len = byte;
		break;
	case 5:
		header->len += byte << 8;
		break;
	case 6:
		header->crc = byte;
		break;
	default:
		break;
	}

	LOG_DBG("byte[%d]: %x", header->idx, byte);

	header->idx++;
	return false;
}

static uint8_t compute_crc(struct nrf_rpc_uart_header *header, struct ring_buf *buf)
{
	/* TODO: implement crc. could be crc8_ccitt() */
	return header->crc;
}

static void process_ringbuf(struct nrf_rpc_uart *uart_config)
{
	struct nrf_rpc_uart_header *header = uart_config->header;

	/* try to parse header */
	while (!ring_buf_is_empty(uart_config->ringbuf) &&
	       !build_header(uart_config)) {
	}

	/* receive the packet data */
	if (build_header(uart_config)) {
		if (ring_buf_size_get(uart_config->ringbuf) >= header->len) {
			if (compute_crc(header, uart_config->ringbuf) == header->crc) {
				LOG_DBG("submit to nrf-rpc");
				k_work_submit(&uart_config->work);
				/* LOG_DBG("early return"); */
				return;
			}
		}
	}
}

/*
 * Read any available characters from UART, and place them in a ring buffer. The
 * ring buffer is in turn processed by process_ringbuf().
 */
void serial_cb(const struct device *uart, void *user_data)
{
	const struct nrf_rpc_tr *transport = (struct nrf_rpc_tr *)user_data;
	struct nrf_rpc_uart *uart_config = transport->ctx;

	if (!uart_irq_update(uart)) {
		return;
	}

	while (uart_irq_rx_ready(uart)) {
		uint8_t byte = 0; /* Have to assign to stop GCC from whining */

		uart_fifo_read(uart, &byte, 1);
		uint32_t ret = ring_buf_put(uart_config->ringbuf, &byte, 1);
		(void)ret; /* Don't warn if logging is disabled */

		LOG_DBG("rx: %x, rb put %u", byte, ret);

		/* Only try to decode if wq item is not pending */
		if (!k_work_busy_get(&uart_config->work)) {
			process_ringbuf(uart_config);
		}
	}
}

static int init(const struct nrf_rpc_tr *transport,
		nrf_rpc_tr_receive_handler_t receive_cb,
		void *context)
{
	LOG_DBG("");

	struct nrf_rpc_uart *uart_config = transport->ctx;

	k_work_init(&uart_config->work, rpc_tr_uart_handler);

	if (uart_config->used) {
		return 0;
	}

	if (receive_cb == NULL) {
		LOG_ERR("No transport receive callback");
		return -NRF_EINVAL;
	}

	/* Initialize UART driver */
	if (!device_is_ready(uart_config->uart)) {
		LOG_ERR("UART device not found!");
		return -NRF_EAGAIN;
	}

	/* Setup nRF RPC UART transport instance */
	uart_config->receive_cb = receive_cb;
	uart_config->context = context;
	uart_config->used = true;

	uart_irq_callback_user_data_set(uart_config->uart, serial_cb, (void *)transport);
	uart_irq_rx_enable(uart_config->uart);

	LOG_DBG("init ok");

	return 0;
}

static int send(const struct nrf_rpc_tr *transport, const uint8_t *data, size_t length)
{
	LOG_DBG("");
	struct nrf_rpc_uart *uart_config = transport->ctx;

	if (!uart_config->used) {
		LOG_ERR("nRF RPC transport is not initialized");
		return -NRF_EFAULT;
	}

	LOG_DBG("Sending %u bytes", length);
	DUMP_LIMITED_DBG(data, length, "Data: ");

	/* Add UART transport header */
	uart_poll_out(uart_config->uart, 'U');
	uart_poll_out(uart_config->uart, 'A');
	uart_poll_out(uart_config->uart, 'R');
	uart_poll_out(uart_config->uart, 'T');
	/* Add length */
	uart_poll_out(uart_config->uart, 0xFF & length);
	uart_poll_out(uart_config->uart, 0xFF & (length >> 8));
	/* Add CRC (not computed for now) */
	uart_poll_out(uart_config->uart, 0);

	for (size_t i = 0; i < length; i++) {
		uart_poll_out(uart_config->uart, data[i]);
	}

	k_free((void *)data);

	LOG_DBG("exit");

	return 0;
}

static void *tx_buf_alloc(const struct nrf_rpc_tr *transport, size_t *size)
{
	LOG_DBG("");
	void *data = NULL;
	struct nrf_rpc_uart *uart_config = transport->ctx;

	if (!uart_config->used) {
		LOG_ERR("nRF RPC transport is not initialized");
		goto error;
	}

	data = k_malloc(*size);
	if (!data) {
		LOG_ERR("Failed to allocate Tx buffer.");
		goto error;
	}

	return data;

error:
	/* It should fail to avoid writing to NULL buffer. */
	k_oops();
	*size = 0;
	return NULL;
}

static void tx_buf_free(const struct nrf_rpc_tr *transport, void *buf)
{
	LOG_DBG("");
	struct nrf_rpc_uart *uart_config = transport->ctx;

	if (!uart_config->used) {
		LOG_ERR("nRF RPC transport is not initialized");
		return;
	}

	k_free(buf);
}

const struct nrf_rpc_tr_api nrf_rpc_uart_api = {
	.init = init,
	.send = send,
	.tx_buf_alloc = tx_buf_alloc,
	.tx_buf_free = tx_buf_free
};
