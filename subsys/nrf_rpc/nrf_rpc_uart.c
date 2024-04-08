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
#include <sys/_stdint.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/crc.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrf_rpc_uart, CONFIG_NRF_RPC_TR_LOG_LEVEL);

enum {
	/* TODO: check if we need handle other special values like XON or XOFF */
	hdlc_char_escape = 0x7d,
	hdlc_char_delimiter = 0x7e,
};

static int hdlc_input_byte(struct nrf_rpc_uart *uart_tr, uint8_t byte)
{
	//uint16_t crc, frame_crc;

	/* TODO: handle frame buffer overflow */

	if (uart_tr->hdlc_state == hdlc_state_escape) {
		uart_tr->frame_buffer[uart_tr->frame_len++] = byte ^ 0x20;
		uart_tr->hdlc_state = hdlc_state_frame_start;
	} else if (byte == hdlc_char_escape) {
		uart_tr->hdlc_state = hdlc_state_escape;
	} else if (byte == hdlc_char_delimiter) {
		uart_tr->hdlc_state = (uart_tr->hdlc_state == hdlc_state_frame_start)
					      ? hdlc_state_frame_found
					      : hdlc_state_unsync;
	} else {
		uart_tr->frame_buffer[uart_tr->frame_len++] = byte;
		uart_tr->hdlc_state = hdlc_state_frame_start;
	}

	if (uart_tr->hdlc_state == hdlc_state_frame_found) {
		//crc = crc16_ccitt(0xffff, frame_buffer, frame_len-1);
		//frame_crc = ((uint16_t)frame_buffer[frame_len-2] << 8) & frame_buffer[frame_len-1];
		/*if (crc == frame_crc) {
			LOG_DBG("frame crc match.");
		} else {
			LOG_DBG("frame crc MISmatch.");
		}*/
		LOG_HEXDUMP_DBG(uart_tr->frame_buffer, uart_tr->frame_len, "Received frame");
	}

	return 0;
}

static void work_handler(struct k_work *work)
{

	struct nrf_rpc_uart *uart_tr = CONTAINER_OF(work, struct nrf_rpc_uart, cb_work);
	const struct nrf_rpc_tr *transport = uart_tr->transport;
	uint8_t *data;
	size_t len;
	int ret;

	len = ring_buf_get_claim(&uart_tr->rx_ringbuf, &data, NRF_RPC_MAX_FRAME_SIZE);

	for (size_t i = 0; i < len; i++) {
		if (hdlc_input_byte(uart_tr, data[i]) == 0) {
			if (uart_tr->hdlc_state == hdlc_state_frame_found) {
				uart_tr->receive_callback(transport, uart_tr->frame_buffer,
							  uart_tr->frame_len, uart_tr->receive_ctx);
				uart_tr->frame_len = 0;
				uart_tr->hdlc_state = hdlc_state_unsync;
			}
		}
	}

	ret = ring_buf_get_finish(&uart_tr->rx_ringbuf, len);
	if (ret < 0) {
		LOG_DBG("Cannot flush ring buffer (%d)", ret);
	}
}

static void serial_cb(const struct device *uart, void *user_data)
{
	struct nrf_rpc_uart *uart_tr = user_data;
	int rx_len, ret;
	uint8_t rx_buffer[8];

	while (uart_irq_update(uart) && uart_irq_rx_ready(uart)) {
		rx_len = uart_fifo_read(uart, rx_buffer, sizeof(rx_buffer));
		if (rx_len <= 0) {
			continue;
		}

		ret = ring_buf_put(&uart_tr->rx_ringbuf, rx_buffer, (uint32_t)rx_len);
		if (ret < rx_len) {
			LOG_ERR("Rx buffer doesn't have enough space. "
				"Bytes pending: %d, written: %d",
				rx_len, ret);
			break;
		}

		k_work_submit_to_queue(&uart_tr->rx_workq, &uart_tr->cb_work);
	}
}

/* TODO: for moving it to uart_tr.*/
K_THREAD_STACK_DEFINE(uart_tr_workq_stack_area, 4096);

static int init(const struct nrf_rpc_tr *transport, nrf_rpc_tr_receive_handler_t receive_cb,
		void *context)
{
	struct nrf_rpc_uart *uart_tr = transport->ctx;

	if(uart_tr->transport != NULL) {
		LOG_DBG("init not needed");
		return 0;
	}

	uart_tr->transport = transport;

	LOG_DBG("init called");

	if (receive_cb == NULL) {
		LOG_ERR("No transport receive callback");
		return -NRF_EINVAL;
	}
	uart_tr->receive_callback = receive_cb;
	uart_tr->receive_ctx = context;

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

	k_sem_init(&uart_tr->uart_tx_sem, 1, 1);

	k_work_queue_init(&uart_tr->rx_workq);
	k_work_queue_start(&uart_tr->rx_workq, uart_tr_workq_stack_area,
					K_THREAD_STACK_SIZEOF(uart_tr_workq_stack_area),
					0, NULL);

	k_work_init(&uart_tr->cb_work, work_handler);
	ring_buf_init(&uart_tr->rx_ringbuf, sizeof(uart_tr->rx_buffer), uart_tr->rx_buffer);
	uart_tr->hdlc_state = hdlc_state_unsync;
	uart_tr->frame_len = 0;
	uart_irq_rx_enable(uart_tr->uart);

	return 0;
}

static int send(const struct nrf_rpc_tr *transport, const uint8_t *data, size_t length)
{
	LOG_HEXDUMP_DBG(data, length, "Sending frame");
	struct nrf_rpc_uart *uart_tr = transport->ctx;
	// uint16_t crc;
	k_sem_take(&uart_tr->uart_tx_sem, K_FOREVER);
	// crc = crc16_ccitt(0xffff, data, length);
	uart_poll_out(uart_tr->uart, hdlc_char_delimiter);

	for(size_t i = 0; i < length; i++) {
		uint8_t byte = data[i];

		if (data[i] == hdlc_char_delimiter || data[i] == hdlc_char_escape) {
			uart_poll_out(uart_tr->uart, hdlc_char_escape);
			byte ^= 0x20;
		}

		uart_poll_out(uart_tr->uart, byte);
	}

//	uart_poll_out(uart_tr->uart, (crc >> 8) & 0xff);
//	uart_poll_out(uart_tr->uart, crc & 0xff);
	uart_poll_out(uart_tr->uart, hdlc_char_delimiter);

	k_free((void*)data);
	k_sem_give(&uart_tr->uart_tx_sem);

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
