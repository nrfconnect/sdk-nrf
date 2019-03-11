/*
 * Copyright (c) 2018 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <device.h>
#include <uart.h>
#include <zephyr.h>
#include <stdio.h>
#include <misc/printk.h>
#include <console.h>
#include <nrfx_power.h>

#define THREAD_STACKSIZE		1024
#define UART_BUF_SIZE			68
#define THREAD_PRIORITY			7

static struct k_sem usb_tx_sem;
static struct k_sem uart_tx_sem;
static K_FIFO_DEFINE(usb_tx_fifo);
static K_FIFO_DEFINE(uart_tx_fifo);

static struct device *usb_dev;
static struct device *uart_dev;

struct uart_data {
	void *fifo_reserved;
	u8_t buffer[UART_BUF_SIZE];
	u16_t len;
};

static void usb_cb(struct device *dev)
{
	uart_irq_update(dev);

	if (uart_irq_rx_ready(dev)) {
		u32_t data_length;
		static struct uart_data *rx;

		if (!rx) {
			rx = k_malloc(sizeof(*rx));
			if (rx) {
				rx->len = 0;
			} else {
				printk("Not able to allocate USB rx buffer\n");
				return;
			}
		}

		data_length = uart_fifo_read(dev, &rx->buffer[rx->len],
					     UART_BUF_SIZE - rx->len);
		rx->len += data_length;

		if (rx->len > 0) {
			k_fifo_put(&uart_tx_fifo, rx);
			rx = NULL;
			k_sem_give(&uart_tx_sem);
		}
	}

	if (uart_irq_tx_ready(dev)) {
		struct uart_data *buf =
			k_fifo_get(&usb_tx_fifo, K_NO_WAIT);
		u16_t written = 0;

		/* Nothing in the FIFO, nothing to send */
		if (!buf) {
			uart_irq_tx_disable(dev);
			return;
		}

		while (buf->len > written) {
			written += uart_fifo_fill(dev,
						  &buf->buffer[written],
						  buf->len - written);
		}

		while (!uart_irq_tx_complete(dev)) {
			/* Wait for the last byte to get
			 * shifted out of the module
			 */
		}

		if (k_fifo_is_empty(&usb_tx_fifo)) {
			uart_irq_tx_disable(dev);
		}

		k_free(buf);
	}
}

void usb_tx_thread(void)
{
	while (1) {
		k_sem_take(&usb_tx_sem, K_FOREVER);
		uart_irq_tx_enable(usb_dev);
	}
}

void uart_tx_thread(void)
{
	while (1) {
		k_sem_take(&uart_tx_sem, K_FOREVER);
		uart_irq_tx_enable(uart_dev);

	}
}

static void uart_cb(struct device *dev)
{
	static struct uart_data *rx;

	uart_irq_update(dev);

	if (uart_irq_rx_ready(dev)) {
		int data_length;

		if (!rx) {
			rx = k_malloc(sizeof(*rx));
			if (rx) {
				rx->len = 0;
			} else {
				char dummy;

				printk("Not able to allocate UART buffer\n");

				/* Drop one byte to avoid spinning in an
				 * eternal loop.
				 */
				uart_fifo_read(dev, &dummy, 1);
				return;
			}
		}

		data_length = uart_fifo_read(dev, &rx->buffer[rx->len],
					     UART_BUF_SIZE - rx->len);
		rx->len += data_length;

		if (rx->len > 0) {
			if ((rx->len == UART_BUF_SIZE) ||
			   (rx->buffer[rx->len - 1] == '\n') ||
			   (rx->buffer[rx->len - 1] == '\r')) {
				k_fifo_put(&usb_tx_fifo, rx);
				rx = NULL;
				k_sem_give(&usb_tx_sem);
			}
		}
	}

	if (uart_irq_tx_ready(dev)) {
		struct uart_data *buf =
			k_fifo_get(&uart_tx_fifo, K_NO_WAIT);
		u16_t written = 0;

		/* Nothing in the FIFO, nothing to send */
		if (!buf) {
			uart_irq_tx_disable(dev);
			return;
		}

		while (buf->len > written) {
			written += uart_fifo_fill(dev,
						  &buf->buffer[written],
						  buf->len - written);
		}

		while (!uart_irq_tx_complete(dev)) {
			/* Wait for the last byte to get
			 * shifted out of the module
			 */
		}

		if (k_fifo_is_empty(&uart_tx_fifo)) {
			uart_irq_tx_disable(dev);
		}

		k_free(buf);
	}
}

static int init_uart(void)
{
	uart_dev = device_get_binding("UART_1");
	if (!uart_dev) {
		return -ENXIO;
	}

	uart_irq_callback_set(uart_dev, uart_cb);
	uart_irq_rx_enable(uart_dev);

	return 0;
}

void power_thread(void)
{
	while (1) {
		if (nrfx_power_usbstatus_get() == NRFX_POWER_USB_STATE_DISCONNECTED) {
			NRF_POWER->SYSTEMOFF = 1;
		}
		k_sleep(100);
	}
}

void main(void)
{
	int ret;
	u32_t baudrate, dtr = 0U;

	k_sem_init(&usb_tx_sem, 0, 1);
	k_sem_init(&uart_tx_sem, 0, 1);

	/* For unknown reasons, we have to sleep ~200 ms here
	 * for the USB to be recognized properly by Windows. We loose early
	 * log information on power cycle because of this.
	 * TODO: Investiate and fix this bug.
	 */
	k_sleep(200);
	init_uart();
	usb_dev = device_get_binding(CONFIG_CDC_ACM_PORT_NAME_0);
	if (!usb_dev) {
		printk("CDC ACM device not found\n");
		return;
	}

	printk("Wait for DTR\n");
	while (1) {
		uart_line_ctrl_get(usb_dev, LINE_CTRL_DTR, &dtr);
		if (dtr)
			break;
	}
	printk("DTR set, start test\n");

	/* They are optional, we use them to test the interrupt endpoint */
	ret = uart_line_ctrl_set(usb_dev, LINE_CTRL_DCD, 1);
	if (ret)
		printk("Failed to set DCD, ret code %d\n", ret);

	ret = uart_line_ctrl_set(usb_dev, LINE_CTRL_DSR, 1);
	if (ret)
		printk("Failed to set DSR, ret code %d\n", ret);

	/* Wait 1 sec for the host to do all settings */
	k_busy_wait(1000000);

	ret = uart_line_ctrl_get(usb_dev, LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret)
		printk("Failed to get baudrate, ret code %d\n", ret);
	else
		printk("Baudrate detected: %d\n", baudrate);

	uart_irq_callback_set(usb_dev, usb_cb);
	uart_irq_rx_enable(usb_dev);

	printk("USB <--> UART bridge is now initialized\n");
}

K_THREAD_DEFINE(usb_tx_thread_id, THREAD_STACKSIZE, usb_tx_thread,
		NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);

K_THREAD_DEFINE(uart_tx_thread_id, THREAD_STACKSIZE, uart_tx_thread,
		NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);

K_THREAD_DEFINE(power_thread_id, THREAD_STACKSIZE, power_thread,
		NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
