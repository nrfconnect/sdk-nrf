/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/logging/log.h>

#include <nrfs_usb.h>

#include <sample_usbd.h>

LOG_MODULE_REGISTER(idle_usb, LOG_LEVEL_INF);

const struct device *const uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

#define RING_BUF_SIZE		  1024
#define TEST_TRANSMISSION_TIME_MS 4000

uint8_t ring_buffer[RING_BUF_SIZE];
struct ring_buf ringbuf;
static bool rx_throttled;
static struct usbd_context *sample_usbd;

K_SEM_DEFINE(dtr_sem, 0, 1);

static inline void print_baudrate(const struct device *dev)
{
	uint32_t baudrate;
	int ret;

	ret = uart_line_ctrl_get(dev, UART_LINE_CTRL_BAUD_RATE, &baudrate);
	if (ret) {
		LOG_WRN("Failed to get baudrate, ret code %d", ret);
	} else {
		LOG_INF("Baudrate %u", baudrate);
	}
}

static void sample_msg_cb(struct usbd_context *const ctx, const struct usbd_msg *msg)
{
	LOG_INF("USBD message: %s", usbd_msg_type_string(msg->type));

	if (usbd_can_detect_vbus(ctx)) {
		if (msg->type == USBD_MSG_VBUS_READY) {
			if (usbd_enable(ctx)) {
				LOG_ERR("Failed to enable device support");
			}
		}

		if (msg->type == USBD_MSG_VBUS_REMOVED) {
			if (usbd_disable(ctx)) {
				LOG_ERR("Failed to disable device support");
			}
		}
	}

	if (msg->type == USBD_MSG_CDC_ACM_CONTROL_LINE_STATE) {
		uint32_t dtr = 0U;

		uart_line_ctrl_get(msg->dev, UART_LINE_CTRL_DTR, &dtr);
		if (dtr) {
			k_sem_give(&dtr_sem);
		}
	}

	if (msg->type == USBD_MSG_CDC_ACM_LINE_CODING) {
		print_baudrate(msg->dev);
	}
}

static int enable_usb_device_next(void)
{
	int err;

	sample_usbd = sample_usbd_init_device(sample_msg_cb);
	if (sample_usbd == NULL) {
		LOG_ERR("Failed to initialize USB device");
		return -ENODEV;
	}

	if (!usbd_can_detect_vbus(sample_usbd)) {
		err = usbd_enable(sample_usbd);
		if (err) {
			LOG_ERR("Failed to enable device support");
			return err;
		}
	}

	LOG_INF("USB device support enabled");

	return 0;
}

static void interrupt_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (!rx_throttled && uart_irq_rx_ready(dev)) {
			int recv_len, rb_len;
			uint8_t buffer[64];
			size_t len = MIN(ring_buf_space_get(&ringbuf), sizeof(buffer));

			if (len == 0) {
				/* Throttle because ring buffer is full */
				uart_irq_rx_disable(dev);
				rx_throttled = true;
				continue;
			}

			recv_len = uart_fifo_read(dev, buffer, len);
			if (recv_len < 0) {
				LOG_ERR("Failed to read UART FIFO");
				recv_len = 0;
			};

			rb_len = ring_buf_put(&ringbuf, buffer, recv_len);
			if (rb_len < recv_len) {
				LOG_ERR("Drop %u bytes", recv_len - rb_len);
			}

			LOG_DBG("tty fifo -> ringbuf %d bytes", rb_len);
			if (rb_len) {
				uart_irq_tx_enable(dev);
			}
		}

		if (uart_irq_tx_ready(dev)) {
			uint8_t buffer[64];
			int rb_len, send_len;

			rb_len = ring_buf_get(&ringbuf, buffer, sizeof(buffer));
			if (!rb_len) {
				LOG_DBG("Ring buffer empty, disable TX IRQ");
				uart_irq_tx_disable(dev);
				continue;
			}

			if (rx_throttled) {
				uart_irq_rx_enable(dev);
				rx_throttled = false;
			}

			send_len = uart_fifo_fill(dev, buffer, rb_len);
			if (send_len < rb_len) {
				LOG_ERR("Drop %d bytes", rb_len - send_len);
			}

			LOG_DBG("ringbuf -> tty fifo %d bytes", send_len);
		}
	}
}

int main(void)
{
	int ret;

	LOG_INF("Hello World! %s", CONFIG_BOARD_TARGET);

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("CDC ACM device not ready");
		return 0;
	}
	LOG_INF("Get UART PM runtime request response: %d", pm_device_runtime_get(uart_dev));

	ret = enable_usb_device_next();
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}

	ring_buf_init(&ringbuf, sizeof(ring_buffer), ring_buffer);

	LOG_INF("Wait for DTR");
	k_sem_take(&dtr_sem, K_FOREVER);
	while (true) {
		uint32_t dtr = 0U;

		uart_line_ctrl_get(uart_dev, UART_LINE_CTRL_DTR, &dtr);
		if (dtr) {
			break;
		}
		/* Give CPU resources to low priority threads. */
		k_sleep(K_MSEC(100));
	}

	/* This code will be reached only
	 * if serial port is opened from the host side
	 */
	LOG_INF("DTR set");

	/* Wait 100ms for the host to do all settings */
	k_msleep(100);

	print_baudrate(uart_dev);
	uart_irq_callback_set(uart_dev, interrupt_handler);

	/* Enable RX interrupts */
	uart_irq_rx_enable(uart_dev);

	/* Give time for the test transmission */
	k_msleep(TEST_TRANSMISSION_TIME_MS);

	/* Disable UART and USB */
	LOG_INF("Disable USB device");
	uart_irq_tx_disable(uart_dev);
	uart_irq_rx_disable(uart_dev);
	uart_rx_disable(uart_dev);
	LOG_INF("Put UART into suspend request response: %d", pm_device_runtime_put(uart_dev));
	usbd_disable(sample_usbd);
	k_msleep(100);

	/* Use NRFS USB disable to switch off USB VREG and disable USB HSPLL.
	 * This is sort of a hack, but without external managed USB hub
	 * it is not possible to disable USB power
	 */
	ret = 1;
	LOG_INF("NRFS USB disable request response: %d", nrfs_usb_disable_request((void *)ret));
	k_msleep(100);

	LOG_INF("Good night");
	k_sleep(K_FOREVER);
}
