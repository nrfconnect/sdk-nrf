/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>

LOG_MODULE_REGISTER(app);

#define BUF_SIZE 64
K_MEM_SLAB_DEFINE_STATIC(uart_slab, BUF_SIZE, 3, 4);

#define TX_DATA_SIZE 5
static const uint8_t tx_buf[TX_DATA_SIZE] = {1, 2, 3, 4, 5};
static uint8_t buffer_print[TX_DATA_SIZE * 5 + 1];

static void to_display_format(const uint8_t *src, size_t size, char *dst)
{
	size_t i;

	for (i = 0; i < size; i++) {
		sprintf(dst + 5 * i, "0x%02x,", src[i]);
	}
}

static void verify_buffers(const uint8_t *tx_data, char *rx_data, size_t size)
{
	if (memcmp(tx_data, rx_data, size)) {
		to_display_format(tx_data, size, buffer_print);
		LOG_ERR("Buffer contents are different: %s", buffer_print);
		to_display_format(rx_data, size, buffer_print);
		LOG_ERR("                           vs: %s", buffer_print);
		__ASSERT(false, "Buffer contents are different");
	}
}

static void uart_irq_handler(const struct device *dev, void *context)
{
	if (uart_irq_tx_ready(dev)) {
		(void)uart_fifo_fill(dev, tx_buf, TX_DATA_SIZE);
		uart_irq_tx_disable(dev);
		LOG_INF("IRQ Tx sent %d bytes", TX_DATA_SIZE);
	}

	if (uart_irq_rx_ready(dev)) {
		uint8_t buf[TX_DATA_SIZE + 1];
		int len = uart_fifo_read(dev, buf, sizeof(buf));

		LOG_INF("IRQ Received data %d bytes", len);
		__ASSERT(len == 1 || len == TX_DATA_SIZE, "Received unexpected data");
		if (len == TX_DATA_SIZE) {
			verify_buffers(tx_buf, buf, TX_DATA_SIZE);
		}
	}
}

static void interrupt_driven(const struct device *dev)
{
	uint8_t c = 0xff;

	uart_irq_callback_set(dev, uart_irq_handler);
	uart_irq_rx_enable(dev);
	while (1) {
		uart_irq_tx_enable(dev);
		k_sleep(K_MSEC(500));

		uart_poll_out(dev, c);
		k_sleep(K_MSEC(100));
	}
}

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	struct device *uart = user_data;
	int err;

	switch (evt->type) {
	case UART_TX_DONE:
		LOG_INF("ASYNC Tx sent %d bytes", evt->data.tx.len);
		break;

	case UART_TX_ABORTED:
		LOG_ERR("ASYNC Tx aborted");
		break;

	case UART_RX_RDY:
		LOG_INF("ASYNC Received data %d bytes", evt->data.rx.len);
		__ASSERT(evt->data.rx.len == 1 || evt->data.rx.len == TX_DATA_SIZE,
			 "Received unexpected data");
		if (evt->data.rx.len == TX_DATA_SIZE) {
			verify_buffers(tx_buf, evt->data.rx.buf + evt->data.rx.offset,
				       TX_DATA_SIZE);
		}
		break;

	case UART_RX_BUF_REQUEST: {
		uint8_t *buf;

		err = k_mem_slab_alloc(&uart_slab, (void **)&buf, K_NO_WAIT);
		__ASSERT(err == 0, "Failed to allocate slab");

		err = uart_rx_buf_rsp(uart, buf, BUF_SIZE);
		__ASSERT(err == 0, "Failed to provide new buffer");
		break;
	}

	case UART_RX_BUF_RELEASED:
		k_mem_slab_free(&uart_slab, (void *)evt->data.rx_buf.buf);
		break;

	case UART_RX_DISABLED:
		break;

	case UART_RX_STOPPED:
		break;
	}
}

static void async(const struct device *lpuart)
{
	int err;
	uint8_t *buf;

	err = k_mem_slab_alloc(&uart_slab, (void **)&buf, K_NO_WAIT);
	__ASSERT(err == 0, "Failed to alloc slab");

	err = uart_callback_set(lpuart, uart_callback, (void *)lpuart);
	__ASSERT(err == 0, "Failed to set callback");

	err = uart_rx_enable(lpuart, buf, BUF_SIZE, 10000);
	__ASSERT(err == 0, "Failed to enable RX");

	while (1) {
		err = uart_tx(lpuart, tx_buf, sizeof(tx_buf), 10000);
		__ASSERT(err == 0, "Failed to initiate transmission");

		k_sleep(K_MSEC(500));

		uart_poll_out(lpuart, tx_buf[0]);
		k_sleep(K_MSEC(100));
	}
}

int main(void)
{
	const struct device *lpuart = DEVICE_DT_GET(DT_NODELABEL(lpuart));

	__ASSERT(device_is_ready(lpuart), "LPUART device not ready");

	if (IS_ENABLED(CONFIG_NRF_SW_LPUART_INT_DRIVEN)) {
		interrupt_driven(lpuart);
	} else {
		async(lpuart);
	}

	return 0;
}
