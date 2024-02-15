/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <nrf_rpc.h>
#include <nrf_rpc_tr.h>
#include <nrf_rpc/nrf_rpc_uart.h>
#include <nrf_rpc_errno.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrf_rpc_uart, CONFIG_NRF_RPC_TR_LOG_LEVEL);

#define MAX_BUF_SIZE 256
static uint8_t buffer[MAX_BUF_SIZE];

volatile int num_interrupts = 0;
volatile int buf_ptr = 0;
static struct nrf_rpc_uart *uart_tr_global;

static const int DELIMITER = 0x7E;
static const int ESCAPE = 0x7D;

static void work_handler(struct k_work *work)
{
	bool found_start=false;
	bool found_stop=false;
	int start = 0;
	int stop = 0;
	LOG_DBG("received data.");
	if (uart_tr_global != NULL) {
		for (size_t i = 0; i < MAX_BUF_SIZE; i++) {
			if (buffer[i] == DELIMITER) {
				if (found_start) {
					found_stop = true;
					stop = i;
				}else {
					found_start = true;
					start = i+1;
				}
			}
		}
		const struct nrf_rpc_tr *transport = uart_tr_global->transport;
		if (found_stop) {
			LOG_DBG("frame found size: %d start: %d stop: %d", stop - start, start, stop);
			uart_tr_global->receive_callback(transport, buffer + start, stop - start, transport->ctx);
			buffer[start] = 0;
			buffer[stop] = 0;
			buf_ptr = 0;
		} else {
			LOG_DBG("frame not found");
		}
	} else {
		LOG_ERR("uart transport not set");
	}
}

K_WORK_DEFINE(work, work_handler);

static void serial_cb(const struct device *dev, void *user_data)
{
	struct nrf_rpc_uart *uart_tr = user_data;
	int read = 0;
	num_interrupts++;
	if (!uart_irq_update(dev)) {
		return;
	}

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	while((read = uart_fifo_read(uart_tr->uart, buffer+buf_ptr, 1)) != 0)
	{
		buf_ptr += read;
	}
	k_work_submit(&work);
}

static int init(const struct nrf_rpc_tr *transport, nrf_rpc_tr_receive_handler_t receive_cb,
		void *context)
{
	struct nrf_rpc_uart *uart_tr = transport->ctx;

	uart_tr->transport = transport;

	LOG_DBG("init called");

	if (receive_cb == NULL) {
		LOG_ERR("No transport receive callback");
		return -NRF_EINVAL;
	}
	uart_tr->receive_callback = receive_cb;
	uart_tr_global = uart_tr;

	if (!device_is_ready(uart_tr->uart)) {
		LOG_ERR("UART device not found!");
		return -NRF_ENOENT;
	}

	/* configure interrupt and callback to receive data */
	int ret = uart_irq_callback_user_data_set(uart_tr->uart, serial_cb, uart_tr);

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			LOG_ERR("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			LOG_ERR("UART device does not support interrupt-driven API\n");
		} else {
			LOG_ERR("Error setting UART callback: %d\n", ret);
		}
		return 0;
	}
	uart_irq_rx_enable(uart_tr->uart);

	return 0;
}

static int send(const struct nrf_rpc_tr *transport, const uint8_t *data, size_t length)
{
	LOG_DBG("Send frame len %d", length);
	struct nrf_rpc_uart *uart_tr = transport->ctx;

	uart_poll_out(uart_tr->uart, DELIMITER);

	for(size_t i = 0; i < length; i++) {
		if(data[i] == DELIMITER || data[i] == ESCAPE) {
			uart_poll_out(uart_tr->uart, ESCAPE);
		}
		uart_poll_out(uart_tr->uart, data[i]);
	}

	uart_poll_out(uart_tr->uart, DELIMITER);

	k_free(data);

	return 0;
}

static void *tx_buf_alloc(const struct nrf_rpc_tr *transport, size_t *size)
{
	void *data = NULL;

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
	k_free(buf);
}

const struct nrf_rpc_tr_api nrf_rpc_uart_service_api = {
	.init = init,
	.send = send,
	.tx_buf_alloc = tx_buf_alloc,
	.tx_buf_free = tx_buf_free,
};
