/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
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
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nrf_rpc_uart, CONFIG_NRF_RPC_TR_LOG_LEVEL);

enum {
	/* TODO: check if we need handle other special values like XON or XOFF */
	hdlc_char_escape = 0x7d,
	hdlc_char_delimiter = 0x7e,
};

#define CRC_SIZE sizeof(uint16_t)

static int hdlc_input_byte(struct nrf_rpc_uart *uart_tr, uint8_t byte)
{
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
		LOG_HEXDUMP_DBG(uart_tr->frame_buffer, uart_tr->frame_len, "Received frame");
		if (uart_tr->frame_len > CRC_SIZE) {
			uart_tr->frame_len -= CRC_SIZE;
		} else {
			uart_tr->frame_len = 0;
			uart_tr->hdlc_state = hdlc_state_unsync;
		}
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
	uint16_t crc_received = 0;
	uint16_t crc_calculated = 0;

	while (!ring_buf_is_empty(&uart_tr->rx_ringbuf)) {
		len = ring_buf_get_claim(&uart_tr->rx_ringbuf, &data, NRF_RPC_MAX_FRAME_SIZE);
		for (size_t i = 0; i < len; i++) {
			if (hdlc_input_byte(uart_tr, data[i]) == 0) {

				if (uart_tr->hdlc_state == hdlc_state_frame_found) {
					crc_received = sys_get_le16(uart_tr->frame_buffer +
							      uart_tr->frame_len);
					crc_calculated = crc16_ccitt(0xffff, uart_tr->frame_buffer,
								     uart_tr->frame_len);
					if (crc_calculated != crc_received) {
						LOG_ERR("Invalid CRC, calculated %x received %x",
							crc_calculated, crc_received);
						uart_tr->frame_len = 0;
						uart_tr->hdlc_state = hdlc_state_unsync;
						return;
					}

					uart_tr->receive_callback(transport, uart_tr->frame_buffer,
								  uart_tr->frame_len,
								  uart_tr->receive_ctx);
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
}

static void serial_cb(const struct device *uart, void *user_data)
{
	struct nrf_rpc_uart *uart_tr = user_data;
	uint32_t rx_len;
	uint8_t *rx_buffer;
	bool new_data = false;

	while (uart_irq_update(uart) && uart_irq_rx_ready(uart)) {
		rx_len = ring_buf_put_claim(&uart_tr->rx_ringbuf, &rx_buffer,
					    uart_tr->rx_ringbuf.size);
		if (rx_len > 0) {
			rx_len = uart_fifo_read(uart, rx_buffer, rx_len);
			int err = ring_buf_put_finish(&uart_tr->rx_ringbuf, rx_len);
			(void)err; /*silence the compiler*/
			__ASSERT_NO_MSG(err == 0);
			if (rx_len <= 0) {
				continue;
			} else {
				new_data = true;
			}
		} else {
			uint8_t dummy;

			/* No space in the ring buffer - consume byte. */
			LOG_WRN("RX ring buffer full.");

			rx_len = uart_fifo_read(uart, &dummy, 1);
		}
	}

	if (new_data) {
		k_work_submit_to_queue(&uart_tr->rx_workq, &uart_tr->cb_work);
	}
}

/* TODO: for moving it to uart_tr.*/
K_THREAD_STACK_DEFINE(uart_tr_workq_stack_area, 4096);

static int init(const struct nrf_rpc_tr *transport, nrf_rpc_tr_receive_handler_t receive_cb,
		void *context)
{
	struct nrf_rpc_uart *uart_tr = transport->ctx;

	if (uart_tr->transport != NULL) {
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
			   K_THREAD_STACK_SIZEOF(uart_tr_workq_stack_area), 0, NULL);

	k_work_init(&uart_tr->cb_work, work_handler);
	ring_buf_init(&uart_tr->rx_ringbuf, sizeof(uart_tr->rx_buffer), uart_tr->rx_buffer);

	uart_tr->hdlc_state = hdlc_state_unsync;
	uart_tr->frame_len = 0;
	uart_irq_rx_enable(uart_tr->uart);

	return 0;
}

static void send_byte(const struct device *dev, uint8_t byte)
{

	if (byte == hdlc_char_delimiter || byte == hdlc_char_escape) {
		uart_poll_out(dev, hdlc_char_escape);
		byte ^= 0x20;
	}

	uart_poll_out(dev, byte);
}

static int send(const struct nrf_rpc_tr *transport, const uint8_t *data, size_t length)
{
	uint8_t crc[2];
	struct nrf_rpc_uart *uart_tr = transport->ctx;

	LOG_HEXDUMP_DBG(data, length, "Sending frame");

	k_sem_take(&uart_tr->uart_tx_sem, K_FOREVER);

	uart_poll_out(uart_tr->uart, hdlc_char_delimiter);

	for (size_t i = 0; i < length; i++) {
		send_byte(uart_tr->uart, data[i]);
	}

	sys_put_le16(crc16_ccitt(0xffff, data, length), crc);
	send_byte(uart_tr->uart, crc[0]);
	send_byte(uart_tr->uart, crc[1]);

	uart_poll_out(uart_tr->uart, hdlc_char_delimiter);

	k_free((void *)data);
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

#define NRF_RPC_UART_INSTANCE(node_id) _CONCAT(nrf_rpc_inst_, DT_DEP_ORD(node_id))

#define NRF_RPC_UART_TRANSPORT_DEFINE(node_id)                                                     \
	struct nrf_rpc_uart NRF_RPC_UART_INSTANCE(node_id) = {                                     \
		.uart = DEVICE_DT_GET(node_id),                                                    \
		.receive_callback = NULL,                                                          \
		.transport = NULL,                                                                 \
	};                                                                                         \
	const struct nrf_rpc_tr NRF_RPC_UART_TRANSPORT(node_id) = {                                \
		.api = &nrf_rpc_uart_service_api,                                                  \
		.ctx = &NRF_RPC_UART_INSTANCE(node_id),                                            \
	};

DT_FOREACH_STATUS_OKAY(nordic_nrf_uarte, NRF_RPC_UART_TRANSPORT_DEFINE);
