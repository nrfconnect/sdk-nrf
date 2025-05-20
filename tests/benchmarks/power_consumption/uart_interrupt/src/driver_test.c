/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

static const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static uint8_t tx_buf[] =  {"a\n\r"};

static void interrupt_driven_uart_callback(const struct device *dev, void *user_data)
{

	uart_irq_update(uart_dev);
	while (uart_irq_is_pending(dev)) {
		if (uart_irq_tx_ready(dev)) {
			uart_fifo_fill(dev, tx_buf, 3);
			uart_irq_tx_disable(uart_dev);
		}
	}
}

void thread_definition(void)
{
	uart_irq_callback_user_data_set(uart_dev, interrupt_driven_uart_callback, NULL);
	while (1) {
		uart_irq_tx_enable(uart_dev);
		k_usleep(600);
	}
}
