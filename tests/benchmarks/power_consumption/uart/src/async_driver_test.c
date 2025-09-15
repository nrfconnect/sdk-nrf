/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
static const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static uint8_t tx_buf[] =  {"a\n\r"};

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		break;
	case UART_TX_ABORTED:
		break;
	case UART_RX_RDY:
		break;
	case UART_RX_BUF_REQUEST:
		break;
	case UART_RX_BUF_RELEASED:
		break;
	case UART_RX_DISABLED:
		break;
	case UART_RX_STOPPED:
		break;
	default:
		break;
	}
}

void thread_definition(void)
{
	int ret;

	uart_callback_set(uart_dev, uart_cb, NULL);
	while (1) {
		ret = uart_tx(uart_dev, tx_buf, sizeof(tx_buf), 0);
		if (ret < 0) {
			printk("Issue with transferring data, terminating thread.");
			return;
		}
		k_usleep(400);
	}
}
