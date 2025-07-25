/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

const struct device *const console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
const struct device *const passtrough_dev = DEVICE_DT_GET(DT_CHOSEN(uart_passthrough));

#define QUEUE_SIZE 16

typedef struct {
	struct k_msgq *tx_queue;
	struct k_msgq *rx_queue;
	const struct device *other_dev;

} uart_pass;

K_MSGQ_DEFINE(console_queue, sizeof(uint8_t), QUEUE_SIZE, 1);
K_MSGQ_DEFINE(passtrough_queue, sizeof(uint8_t), QUEUE_SIZE, 1);

static void uart_tx_interrupt_service(const struct device *dev, uart_pass *test_data)
{
	uint8_t tx_data;
	uint32_t items_in_queue = k_msgq_num_used_get(test_data->tx_queue);

	for (int i = 0; i < items_in_queue; i++) {
		k_msgq_get(test_data->tx_queue, &tx_data, K_NO_WAIT);
		uart_fifo_fill(dev, &tx_data, 1);
	}
	uart_irq_tx_disable(dev);
}

static void uart_rx_interrupt_service(const struct device *dev, uart_pass *test_data)
{
	uint8_t rx_data[QUEUE_SIZE];
	int rx_data_length = 0;

	rx_data_length = uart_fifo_read(dev, rx_data, QUEUE_SIZE);
	for (int i = 0; i < rx_data_length; i++) {
		k_msgq_put(test_data->rx_queue, &rx_data[i], K_NO_WAIT);
	}
	uart_irq_tx_enable(test_data->other_dev);
}

static void uart_isr_handler(const struct device *dev, void *user_data)
{
	uart_pass *test_data = (uart_pass *)user_data;

	uart_irq_update(dev);
	while (uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			uart_rx_interrupt_service(dev, test_data);
		}
		if (uart_irq_tx_ready(dev)) {
			uart_tx_interrupt_service(dev, test_data);
		}
	}
}

int main(void)
{
	int err;

	uart_pass console_dev_data;
	uart_pass passtrough_dev_data;

	console_dev_data.rx_queue = &console_queue;
	console_dev_data.tx_queue = &passtrough_queue;
	console_dev_data.other_dev = passtrough_dev;

	err = uart_irq_callback_set(console_dev, uart_isr_handler);
	printk("uart_irq_callback_set(console_dev) err=%d\n", err);
	err = uart_irq_callback_user_data_set(console_dev, uart_isr_handler,
					      (void *)&console_dev_data);
	printk("uart_irq_callback_user_data_set(console_dev) err=%d\n", err);

	passtrough_dev_data.rx_queue = &passtrough_queue;
	passtrough_dev_data.tx_queue = &console_queue;
	passtrough_dev_data.other_dev = console_dev;

	err = uart_irq_callback_set(passtrough_dev, uart_isr_handler);
	printk("uart_irq_callback_set(passtrough_dev) err=%d\n", err);
	err = uart_irq_callback_user_data_set(passtrough_dev, uart_isr_handler,
					      (void *)&passtrough_dev_data);
	printk("uart_irq_callback_user_data_set(passtrough_devle_dev) err=%d\n", err);

	k_msleep(10);
	printk("Ready\n");

	uart_irq_rx_enable(console_dev);
	uart_irq_rx_enable(passtrough_dev);

	k_sleep(K_FOREVER);

	return 0;
}
