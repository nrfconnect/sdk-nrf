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

typedef struct {
	uint8_t *rx_buffer;
	uint8_t *tx_buffer;
	const struct device *other_dev;

} uart_pass;

/*
 * Note: this simple test uses one byte buffer,
 * CPU must be able to receive and send one character,
 * before another one is received
 * This limits the usable buadrates range
 */

static uint8_t console_buf[1];
static uint8_t passthrough_buf[1];

void uart_tx_interrupt_service(const struct device *dev, uart_pass *test_data)
{
	uart_fifo_fill(dev, test_data->tx_buffer, 1);
	uart_irq_tx_disable(dev);
}

void uart_rx_interrupt_service(const struct device *dev, uart_pass *test_data)
{
	int rx_data_length = 0;

	do {
		rx_data_length = uart_fifo_read(dev, test_data->rx_buffer, 1);
	} while (rx_data_length);
	uart_irq_tx_enable(test_data->other_dev);
}

void uart_isr_handler(const struct device *dev, void *user_data)
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

	console_dev_data.rx_buffer = console_buf;
	console_dev_data.tx_buffer = passthrough_buf;
	console_dev_data.other_dev = passtrough_dev;

	err = uart_irq_callback_set(console_dev, uart_isr_handler);
	printk("uart_irq_callback_set(console_dev) err=%d\n", err);
	err = uart_irq_callback_user_data_set(console_dev, uart_isr_handler,
					      (void *)&console_dev_data);
	printk("uart_irq_callback_user_data_set(console_dev) err=%d\n", err);

	passtrough_dev_data.rx_buffer = passthrough_buf;
	passtrough_dev_data.tx_buffer = console_buf;
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
