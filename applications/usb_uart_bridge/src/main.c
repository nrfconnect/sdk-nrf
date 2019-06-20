/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <uart.h>
#include <nrfx.h>
#include <string.h>

/* Overriding weak function to set iSerial runtime. */
u8_t *usb_update_sn_string_descriptor(void)
{
	static u8_t buf[] = "PCA20035_12PLACEHLDRS";

	snprintk(&buf[9], 13, "%04X%08X",
		(uint32_t)(NRF_FICR->DEVICEADDR[1] & 0x0000FFFF)|0x0000C000,
		(uint32_t)NRF_FICR->DEVICEADDR[0]);

	return (u8_t *)&buf;
}

#define THREAD_STACKSIZE		1024
#define UART_BUF_SIZE			58
#define THREAD_PRIORITY			7

static K_FIFO_DEFINE(usb_0_tx_fifo);
static K_FIFO_DEFINE(usb_1_tx_fifo);
static K_FIFO_DEFINE(uart_0_tx_fifo);
static K_FIFO_DEFINE(uart_1_tx_fifo);

struct uart_data {
	void *fifo_reserved;
	u8_t buffer[UART_BUF_SIZE];
	u16_t len;
};

static struct serial_dev {
	struct device *dev;
	void *peer;
	struct k_fifo *fifo;
	struct k_sem sem;
	struct uart_data *rx;
} devs[4];

static void uart_interrupt_handler(void *user_data)
{
	struct serial_dev *dev_data = user_data;
	struct device *dev = dev_data->dev;
	struct serial_dev *peer_dev_data = (struct serial_dev *)dev_data->peer;
	struct uart_data **rx = &dev_data->rx;

	uart_irq_update(dev);

	while (uart_irq_rx_ready(dev)) {
		int data_length;

		if (!(*rx)) {
			(*rx) = k_malloc(sizeof(*(*rx)));
			if ((*rx)) {
				(*rx)->len = 0;
			} else {
				char dummy;

				printk("Not able to allocate UART buffer, ");
				printk("dropping oldest entry\n");

				/* Drop the oldest entry. */
				(void)k_fifo_get(dev_data->fifo, K_NO_WAIT);

				/* Drop one byte to avoid spinning in an
				 * eternal loop.
				 */
				uart_fifo_read(dev, &dummy, 1);
				return;
			}
		}

		data_length = uart_fifo_read(dev, &(*rx)->buffer[(*rx)->len],
					     UART_BUF_SIZE - (*rx)->len);
		(*rx)->len += data_length;

		if ((*rx)->len > 0) {
			if (((*rx)->len == UART_BUF_SIZE) ||
			   ((*rx)->buffer[(*rx)->len - 1] == '\n') ||
			   ((*rx)->buffer[(*rx)->len - 1] == '\r') ||
			   ((*rx)->buffer[(*rx)->len - 1] == '\0')) {
				k_fifo_put(peer_dev_data->fifo, (*rx));
				k_sem_give(&peer_dev_data->sem);

				(*rx) = NULL;
			}
		}
	}

	if (uart_irq_tx_ready(dev)) {
		struct uart_data *buf = k_fifo_get(dev_data->fifo, K_NO_WAIT);
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

		if (k_fifo_is_empty(dev_data->fifo)) {
			uart_irq_tx_disable(dev);
		}

		k_free(buf);
	}
}

void power_thread(void)
{
	while (1) {
		u32_t status = NRF_POWER->USBREGSTATUS;

		if (0 == (status & POWER_USBREGSTATUS_VBUSDETECT_Msk)) {
			NRF_POWER->SYSTEMOFF = 1;
		}
		k_sleep(100);
	}
}

void main(void)
{
	int ret;
	struct serial_dev *usb_0_dev_data = &devs[0];
	struct serial_dev *usb_1_dev_data = &devs[1];
	struct serial_dev *uart_0_dev_data = &devs[2];
	struct serial_dev *uart_1_dev_data = &devs[3];
	struct device *usb_0_dev, *usb_1_dev, *uart_0_dev, *uart_1_dev;

	usb_0_dev = device_get_binding("CDC_ACM_0");
	if (!usb_0_dev) {
		printk("CDC ACM device not found\n");
		return;
	}

	usb_1_dev = device_get_binding("CDC_ACM_1");
	if (!usb_1_dev) {
		printk("CDC ACM device not found\n");
		return;
	}

	uart_0_dev = device_get_binding("UART_0");
	if (!uart_0_dev) {
		printk("UART 0 init failed\n");
	}

	uart_1_dev = device_get_binding("UART_1");
	if (!uart_1_dev) {
		printk("UART 1 init failed\n");
	}

	usb_0_dev_data->dev = usb_0_dev;
	usb_0_dev_data->fifo = &usb_0_tx_fifo;
	usb_0_dev_data->peer = uart_0_dev_data;

	usb_1_dev_data->dev = usb_1_dev;
	usb_1_dev_data->fifo = &usb_1_tx_fifo;
	usb_1_dev_data->peer = uart_1_dev_data;

	uart_0_dev_data->dev = uart_0_dev;
	uart_0_dev_data->fifo = &uart_0_tx_fifo;
	uart_0_dev_data->peer = usb_0_dev_data;

	uart_1_dev_data->dev = uart_1_dev;
	uart_1_dev_data->fifo = &uart_1_tx_fifo;
	uart_1_dev_data->peer = usb_1_dev_data;

	k_sem_init(&usb_0_dev_data->sem, 0, 1);
	k_sem_init(&usb_1_dev_data->sem, 0, 1);
	k_sem_init(&uart_0_dev_data->sem, 0, 1);
	k_sem_init(&uart_1_dev_data->sem, 0, 1);

	/* For unknown reasons, we have to sleep ~200 ms here
	 * for the USB to be recognized properly by Windows. We loose early
	 * log information on power cycle because of this.
	 * TODO: Investiate and fix this bug.
	 */
	k_sleep(200);

	/* Wait 1 sec for the host to do all settings */
	k_busy_wait(1000000);

	uart_irq_callback_user_data_set(usb_0_dev, uart_interrupt_handler,
		usb_0_dev_data);
	uart_irq_callback_user_data_set(usb_1_dev, uart_interrupt_handler,
		usb_1_dev_data);
	uart_irq_callback_user_data_set(uart_0_dev, uart_interrupt_handler,
		uart_0_dev_data);
	uart_irq_callback_user_data_set(uart_1_dev, uart_interrupt_handler,
		uart_1_dev_data);

	uart_irq_rx_enable(usb_0_dev);
	uart_irq_rx_enable(usb_1_dev);
	uart_irq_rx_enable(uart_0_dev);
	uart_irq_rx_enable(uart_1_dev);

	printk("USB <--> UART bridge is now initialized\n");

	struct k_poll_event events[4] = {
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&usb_0_dev_data->sem, 0),
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&usb_1_dev_data->sem, 0),
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&uart_0_dev_data->sem, 0),
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&uart_1_dev_data->sem, 0),
	};

	while (1) {
		ret = k_poll(events, ARRAY_SIZE(events), K_FOREVER);
		if (ret != 0) {
			continue;
		}

		if (events[0].state == K_POLL_TYPE_SEM_AVAILABLE) {
			events[0].state = K_POLL_STATE_NOT_READY;
			k_sem_take(&usb_0_dev_data->sem, K_NO_WAIT);
			uart_irq_tx_enable(usb_0_dev);
		} else if (events[1].state == K_POLL_TYPE_SEM_AVAILABLE) {
			events[1].state = K_POLL_STATE_NOT_READY;
			k_sem_take(&usb_1_dev_data->sem, K_NO_WAIT);
			uart_irq_tx_enable(usb_1_dev);
		} else if (events[2].state == K_POLL_TYPE_SEM_AVAILABLE) {
			events[2].state = K_POLL_STATE_NOT_READY;
			k_sem_take(&uart_0_dev_data->sem, K_NO_WAIT);
			uart_irq_tx_enable(uart_0_dev);
		} else if (events[3].state == K_POLL_TYPE_SEM_AVAILABLE) {
			events[3].state = K_POLL_STATE_NOT_READY;
			k_sem_take(&uart_1_dev_data->sem, K_NO_WAIT);
			uart_irq_tx_enable(uart_1_dev);
		}
	}
}

K_THREAD_DEFINE(power_thread_id, THREAD_STACKSIZE, power_thread,
		NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
