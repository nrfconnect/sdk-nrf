/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#define BUF_SIZE 1
static char rx_buf[BUF_SIZE];

static K_SEM_DEFINE(my_uart_sem, 0, 1);

static const struct device *const uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		break;
	case UART_TX_ABORTED:
		break;
	case UART_RX_RDY:
		k_sem_give(&my_uart_sem);
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

int main(void)
{
	int ret;

	ret = device_is_ready(uart_dev);
	if (ret < 0) {
		printk("device_is_ready: %d\n", ret);
		return 0;
	}

	ret = uart_callback_set(uart_dev, uart_cb, NULL);
	if (ret < 0) {
		printk("uart_callback_set: %d\n", ret);
		return 0;
	}

	while (1) {
		uart_rx_enable(uart_dev, rx_buf, BUF_SIZE, 0);
		if (ret < 0) {
			printk("uart_rx_enable: %d\n", ret);
			return 0;
		}
		if (k_sem_take(&my_uart_sem, K_FOREVER) != 0) {
			printk("Failed to take a semaphore\n");
			return 0;
		}
		k_busy_wait(1000000);
		k_sem_reset(&my_uart_sem);
	}

	return 0;
}
