/*

* Copyright (c) 2012-2014 Wind River Systems, Inc.

*

* SPDX-License-Identifier: Apache-2.0

*/

#include <zephyr.h>
#include <misc/printk.h>
#include <uart.h>

static u8_t uart_buf[1024];

void uart_cb(struct device *x)
{
	uart_irq_update(x);
	int data_length = 0;

	if (uart_irq_rx_ready(x)) {
		data_length = uart_fifo_read(x, uart_buf, sizeof(uart_buf));
		uart_buf[data_length] = 0;
	}
	printk("%s", uart_buf);
}

void main(void)
{
	struct device *uart = device_get_binding("UART_0");

	uart_irq_callback_set(uart, uart_cb);
	uart_irq_rx_enable(uart);
	printk("UART loopback start!\n");
	while (1) {
		k_cpu_idle();
	}
}
