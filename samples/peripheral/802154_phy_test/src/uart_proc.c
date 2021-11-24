/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
/* Purpose: UART configuring and processing */

/* UART handler waits for a string with new line symbol in the end. The handler reads a packet
 * from UARTE driver per a byte and looks for a new line symbol or waits until a timer is expired
 * to process received sequence without new line symbol. Then the packet is pushed to the library.
 */

#include <assert.h>
#include <stdint.h>

#include <drivers/uart.h>
#include <sys/ring_buffer.h>

#include "comm_proc.h"
#include "uart_proc.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(uart);

static struct text_proc_s text_proc_uart;
static struct k_timer uart_timer;
static const struct device *uart_dev;

RING_BUF_DECLARE(uart_rb, 10240);

static void uart_irq_handler(const struct device *dev, void *context)
{
	uint8_t *data_tx = NULL;

	uart_irq_update(dev);
	if (uart_irq_tx_ready(dev)) {
		int nr_bytes_read =
			ring_buf_get_claim(&uart_rb, &data_tx, CONFIG_UART_0_NRF_TX_BUFFER_SIZE);
		int sent = uart_fifo_fill(dev, data_tx, nr_bytes_read);

		ring_buf_get_finish(&uart_rb, sent);
		if (ring_buf_is_empty(&uart_rb)) {
			uart_irq_tx_disable(dev);
		}
	}

	if (uart_irq_rx_ready(dev)) {
		uint8_t buf[10] = {0};
		int len = uart_fifo_read(dev, buf, sizeof(buf));

		if (len) {
			/* Call this */
			comm_input_process(&text_proc_uart, buf, UART_BYTES_TO_READ);
		}
	}
}

void uart_init(void)
{
	LOG_INF("Init");

	uart_dev = device_get_binding("UART_0");
	__ASSERT(uart_dev, "Failed to get the device");
	uart_irq_callback_user_data_set(uart_dev, uart_irq_handler, NULL);
	uart_irq_rx_enable(uart_dev);

	text_proc_uart = (struct text_proc_s){ 0 };
	text_proc_uart.timer = uart_timer;
	k_timer_init(&text_proc_uart.timer, comm_input_timeout_handler, NULL);
	k_work_init(&text_proc_uart.work, comm_text_processor_fn);
}

int32_t uart_send(const uint8_t *pkt, ptt_pkt_len_t len, bool add_crlf)
{
	while (ring_buf_space_get(&uart_rb) < len) {
		LOG_WRN("queue full, waiting for free space");
		k_sleep(K_MSEC(1));
	}

	uart_irq_tx_disable(uart_dev);
	ring_buf_put(&uart_rb, pkt, len);
	if (add_crlf) {
		ring_buf_put(&uart_rb, COMM_APPENDIX, COMM_APPENDIX_SIZE);
	}
	uart_irq_tx_enable(uart_dev);

	return len;
}
