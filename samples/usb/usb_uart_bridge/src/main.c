/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/uart.h>
#include <nrfx.h>
#include <string.h>
#include <hal/nrf_power.h>
#include <power/reboot.h>

/* Overriding weak function to set iSerial runtime. */
u8_t *usb_update_sn_string_descriptor(void)
{
	static u8_t buf[] = "PCA20035_12PLACEHLDRS";

	snprintk(&buf[9], 13, "%04X%08X",
		(uint32_t)(NRF_FICR->DEVICEADDR[1] & 0x0000FFFF)|0x0000C000,
		(uint32_t)NRF_FICR->DEVICEADDR[0]);

	return (u8_t *)&buf;
}

#define POWER_THREAD_STACKSIZE		CONFIG_IDLE_STACK_SIZE
#define POWER_THREAD_PRIORITY		K_LOWEST_APPLICATION_THREAD_PRIO

/* Heap block space is always one of 2^(2n) for n from 3 to 7.
 * (see reference/kernel/memory/heap.html for more info)
 * Here, we target 64-byte blocks. Since we want to fit one struct uart_data
 * into each block, the block layout becomes:
 * 16 bytes: reserved by the Zephyr heap block descriptor (not in struct)
 *  4 bytes: reserved by the Zephyr FIFO (in struct)
 * 40 bytes: UART data buffer (in struct)
 *  4 bytes: length field (in struct, padded for alignment)
 */
#define UART_BUF_SIZE 40

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

/* Frees data for incoming transmission on device blocked by full heap. */
static int oom_free(struct serial_dev *sd)
{
	struct serial_dev *peer_sd = (struct serial_dev *)sd->peer;
	struct uart_data *buf;

	/* First, try to free from FIFO of peer device (blocked stream) */
	buf = k_fifo_get(peer_sd->fifo, K_NO_WAIT);
	if (buf) {
		k_free(buf);
		return 0;
	}

	/* Then, try FIFO of the receiving device (reverse of blocked stream) */
	buf = k_fifo_get(sd->fifo, K_NO_WAIT);
	if (buf) {
		k_free(buf);
		return 0;
	}

	/* Finally, try all of them */
	for (int i = 0; i < ARRAY_SIZE(devs); i++) {
		buf = k_fifo_get(sd->fifo, K_NO_WAIT);
		if (buf) {
			k_free(buf);
			return 0;
		}
	}

	return -1; /* Was not able to free any heap memory */
}

static void uart_interrupt_handler(void *user_data)
{
	struct serial_dev *sd = user_data;
	struct device *dev = sd->dev;
	struct serial_dev *peer_sd = (struct serial_dev *)sd->peer;

	uart_irq_update(dev);

	while (uart_irq_rx_ready(dev)) {
		int data_length;

		while (!sd->rx) {
			sd->rx = k_malloc(sizeof(*sd->rx));
			if (sd->rx) {
				sd->rx->len = 0;
			} else {
				int err = oom_free(sd);

				if (err) {
					printk("Could not free memory. Rebooting.\n");
					sys_reboot(SYS_REBOOT_COLD);
				}
			}
		}

		data_length = uart_fifo_read(dev, &sd->rx->buffer[sd->rx->len],
					   UART_BUF_SIZE - sd->rx->len);
		sd->rx->len += data_length;

		if (sd->rx->len > 0) {
			if ((sd->rx->len == UART_BUF_SIZE) ||
			   (sd->rx->buffer[sd->rx->len - 1] == '\n') ||
			   (sd->rx->buffer[sd->rx->len - 1] == '\r') ||
			   (sd->rx->buffer[sd->rx->len - 1] == '\0')) {
				k_fifo_put(peer_sd->fifo, sd->rx);
				k_sem_give(&peer_sd->sem);

				sd->rx = NULL;
			}
		}
	}

	if (uart_irq_tx_ready(dev)) {
		struct uart_data *buf = k_fifo_get(sd->fifo, K_NO_WAIT);
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

		if (k_fifo_is_empty(sd->fifo)) {
			uart_irq_tx_disable(dev);
		}

		k_free(buf);
	}
}

void power_thread(void)
{
	while (1) {
		if (!nrf_power_usbregstatus_vbusdet_get(NRF_POWER)) {
			nrf_power_system_off(NRF_POWER);
		}
		k_sleep(100);
	}
}

void main(void)
{
	int ret;
	struct serial_dev *usb_0_sd = &devs[0];
	struct serial_dev *usb_1_sd = &devs[1];
	struct serial_dev *uart_0_sd = &devs[2];
	struct serial_dev *uart_1_sd = &devs[3];
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

	usb_0_sd->dev = usb_0_dev;
	usb_0_sd->fifo = &usb_0_tx_fifo;
	usb_0_sd->peer = uart_0_sd;

	usb_1_sd->dev = usb_1_dev;
	usb_1_sd->fifo = &usb_1_tx_fifo;
	usb_1_sd->peer = uart_1_sd;

	uart_0_sd->dev = uart_0_dev;
	uart_0_sd->fifo = &uart_0_tx_fifo;
	uart_0_sd->peer = usb_0_sd;

	uart_1_sd->dev = uart_1_dev;
	uart_1_sd->fifo = &uart_1_tx_fifo;
	uart_1_sd->peer = usb_1_sd;

	k_sem_init(&usb_0_sd->sem, 0, 1);
	k_sem_init(&usb_1_sd->sem, 0, 1);
	k_sem_init(&uart_0_sd->sem, 0, 1);
	k_sem_init(&uart_1_sd->sem, 0, 1);

	uart_irq_callback_user_data_set(usb_0_dev, uart_interrupt_handler,
		usb_0_sd);
	uart_irq_callback_user_data_set(usb_1_dev, uart_interrupt_handler,
		usb_1_sd);
	uart_irq_callback_user_data_set(uart_0_dev, uart_interrupt_handler,
		uart_0_sd);
	uart_irq_callback_user_data_set(uart_1_dev, uart_interrupt_handler,
		uart_1_sd);

	uart_irq_rx_enable(usb_0_dev);
	uart_irq_rx_enable(usb_1_dev);
	uart_irq_rx_enable(uart_0_dev);
	uart_irq_rx_enable(uart_1_dev);

	printk("USB <--> UART bridge is now initialized\n");

	struct k_poll_event events[4] = {
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&usb_0_sd->sem, 0),
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&usb_1_sd->sem, 0),
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&uart_0_sd->sem, 0),
		K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
						K_POLL_MODE_NOTIFY_ONLY,
						&uart_1_sd->sem, 0),
	};

	while (1) {
		ret = k_poll(events, ARRAY_SIZE(events), K_FOREVER);
		if (ret != 0) {
			continue;
		}

		if (events[0].state == K_POLL_TYPE_SEM_AVAILABLE) {
			events[0].state = K_POLL_STATE_NOT_READY;
			k_sem_take(&usb_0_sd->sem, K_NO_WAIT);
			uart_irq_tx_enable(usb_0_dev);
		} else if (events[1].state == K_POLL_TYPE_SEM_AVAILABLE) {
			events[1].state = K_POLL_STATE_NOT_READY;
			k_sem_take(&usb_1_sd->sem, K_NO_WAIT);
			uart_irq_tx_enable(usb_1_dev);
		} else if (events[2].state == K_POLL_TYPE_SEM_AVAILABLE) {
			events[2].state = K_POLL_STATE_NOT_READY;
			k_sem_take(&uart_0_sd->sem, K_NO_WAIT);
			uart_irq_tx_enable(uart_0_dev);
		} else if (events[3].state == K_POLL_TYPE_SEM_AVAILABLE) {
			events[3].state = K_POLL_STATE_NOT_READY;
			k_sem_take(&uart_1_sd->sem, K_NO_WAIT);
			uart_irq_tx_enable(uart_1_dev);
		}
	}
}

K_THREAD_DEFINE(power_thread_id, POWER_THREAD_STACKSIZE, power_thread,
		NULL, NULL, NULL, POWER_THREAD_PRIORITY, 0, K_NO_WAIT);
