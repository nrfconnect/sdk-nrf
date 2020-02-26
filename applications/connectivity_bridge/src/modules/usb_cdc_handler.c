/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <stdio.h>
#include <zephyr/types.h>
#include <drivers/uart.h>
#include <usb/usb_device.h>

#define MODULE usb_cdc
#include "module_state_event.h"
#include "peer_conn_event.h"
#include "cdc_data_event.h"
#include "uart_data_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_BRIDGE_CDC_LOG_LEVEL);

#define CDC_DEVICE_COUNT CONFIG_USB_CDC_ACM_DEVICE_COUNT
#define CDC_DEVICE_NAME_TEMPLATE CONFIG_USB_CDC_ACM_DEVICE_NAME "_%d"

#define USB_CDC_DTR_POLL_MS 500
#define USB_CDC_RX_BLOCK_SIZE CONFIG_BRIDGE_BUF_SIZE
#define USB_CDC_RX_BLOCK_COUNT (CDC_DEVICE_COUNT * 3)
#define USB_CDC_SLAB_ALIGNMENT 4

static void cdc_dtr_timer_handler(struct k_timer *timer);
static void cdc_dtr_work_handler(struct k_work *work);

static K_TIMER_DEFINE(cdc_dtr_timer, cdc_dtr_timer_handler, NULL);
static K_WORK_DEFINE(cdc_dtr_work, cdc_dtr_work_handler);
/* Incoming data from any CDC instance is copied into a block from this slab */
K_MEM_SLAB_DEFINE(cdc_rx_slab, USB_CDC_RX_BLOCK_SIZE, USB_CDC_RX_BLOCK_COUNT, USB_CDC_SLAB_ALIGNMENT);

static struct device *devices[CDC_DEVICE_COUNT];
static u32_t cdc_ready[CDC_DEVICE_COUNT];

static u8_t overflow_buf[64];

static void cdc_dtr_timer_handler(struct k_timer *timer)
{
	k_work_submit(&cdc_dtr_work);
}

static void poll_dtr(void)
{
	for (int i = 0; i < CDC_DEVICE_COUNT; ++i) {
		int err;
		u32_t cdc_val;

		err = uart_line_ctrl_get(devices[i],
					 UART_LINE_CTRL_DTR,
					 &cdc_val);
		if (err) {
			LOG_WRN("uart_line_ctrl_get(DTR): %d", err);
			continue;
		}

		if (cdc_val != cdc_ready[i]) {
			u32_t baudrate;

			err = uart_line_ctrl_get(devices[i],
						UART_LINE_CTRL_BAUD_RATE,
						&baudrate);
			if (err) {
				LOG_WRN("uart_line_ctrl_get(BAUD_RATE): %d", err);
				continue;
			}

			struct peer_conn_event *event = new_peer_conn_event();

			event->peer_id = PEER_ID_USB;
			event->dev_idx = i;
			event->baudrate = baudrate;
			event->conn_state =
				cdc_val == 0 ? PEER_STATE_DISCONNECTED : PEER_STATE_CONNECTED;
			EVENT_SUBMIT(event);

			cdc_ready[i] = cdc_val;
		}
	}
}

static void cdc_dtr_work_handler(struct k_work *work)
{
	poll_dtr();
}

static void cdc_uart_interrupt_handler(void *user_data)
{
	int dev_idx = (int) user_data;
	struct device *dev = devices[dev_idx];

	uart_irq_update(dev);

	while (uart_irq_rx_ready(dev)) {
		void *rx_buf;
		int err;
		int data_length;

		if (cdc_ready[dev_idx] == 0) {
			/* Peer started sending data before timed COM port state poll */
			poll_dtr();
		}

		err = k_mem_slab_alloc(&cdc_rx_slab, &rx_buf, K_NO_WAIT);
		if (err) {
			data_length = uart_fifo_read(
				dev,
				overflow_buf,
				sizeof(overflow_buf));
			LOG_WRN("CDC_%d RX overflow", dev_idx);
		} else {
			data_length = uart_fifo_read(
				dev,
				rx_buf,
				USB_CDC_RX_BLOCK_SIZE);

			if (data_length) {
				struct cdc_data_event *event = new_cdc_data_event();

				event->dev_idx = dev_idx;
				event->buf = rx_buf;
				event->len = data_length;
				EVENT_SUBMIT(event);
			} else {
				k_mem_slab_free(&cdc_rx_slab, &rx_buf);
			}
		}
	}
}

static void enable_rx_irq(int dev_idx)
{
	uart_irq_callback_user_data_set(
		devices[dev_idx],
		cdc_uart_interrupt_handler,
		(void *)dev_idx);
	uart_irq_rx_enable(devices[dev_idx]);
}

static void usbd_status(enum usb_dc_status_code cb_status, const u8_t *param)
{
	switch (cb_status) {
	case USB_DC_ERROR:
		LOG_ERR("USB_DC_ERROR");
		break;
	case USB_DC_RESET:
		LOG_DBG("USB_DC_RESET");
		break;
	case USB_DC_CONNECTED:
		LOG_DBG("USB_DC_CONNECTED");
		module_set_state(MODULE_STATE_READY);
		break;
	case USB_DC_CONFIGURED:
		LOG_DBG("USB_DC_CONFIGURED");
		break;
	case USB_DC_DISCONNECTED:
		LOG_DBG("USB_DC_DISCONNECTED");
		module_set_state(MODULE_STATE_STANDBY);
		break;
	case USB_DC_SUSPEND:
		LOG_DBG("USB_DC_SUSPEND");
		break;
	case USB_DC_RESUME:
		LOG_DBG("USB_DC_RESUME");
		break;
	default:
		break;
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_uart_data_event(eh)) {
		const struct uart_data_event *event =
			cast_uart_data_event(eh);
		int tx_written;

		if (event->dev_idx >= CDC_DEVICE_COUNT) {
			return false;
		}

		if (cdc_ready[event->dev_idx] == 0) {
			return false;
		}

		tx_written = uart_fifo_fill(
			devices[event->dev_idx],
			event->buf,
			event->len);

		if (tx_written != event->len) {
			LOG_DBG("UART_%d->CDC_%d overflow",
				event->dev_idx,
				event->dev_idx);
		}

		return false;
	}

	if (is_cdc_data_event(eh)) {
		const struct cdc_data_event *event =
			cast_cdc_data_event(eh);

		/* All subscribers have gotten a chance to copy data at this point */
		k_mem_slab_free(&cdc_rx_slab, (void **) &event->buf);

		return true;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

#if CONFIG_BRIDGE_MSC_ENABLE
		if (check_state(event, MODULE_ID(fs_handler), MODULE_STATE_STANDBY)) {
			/* Enable USBD once file system is populated */
#else
		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			/* Enable USBD right away */
#endif
			int err;

			err = usb_enable(usbd_status);
			if (err) {
				LOG_ERR("usb_enable: %d", err);
				return false;
			}
			for (int i = 0; i < CDC_DEVICE_COUNT; ++i) {
				char dev_name[sizeof(CDC_DEVICE_NAME_TEMPLATE) + 1];

				err = snprintf(
						dev_name,
						sizeof(dev_name),
						CDC_DEVICE_NAME_TEMPLATE,
						i);
				if (err <= 0 || err > sizeof(dev_name)) {
					LOG_ERR("Device name format error");
					continue;
				}

				cdc_ready[i] = 0;
				devices[i] = device_get_binding(dev_name);
				if (devices[i]) {
					enable_rx_irq(i);
					LOG_DBG("%s available", log_strdup(dev_name));
				} else {
					LOG_ERR("%s not available", log_strdup(dev_name));
				}
			}

			/* CDC port status needs to be polled (no irq/callback) */
			/* (keeping serial ports open has a power penalty) */
			k_timer_start(
				&cdc_dtr_timer,
				K_MSEC(USB_CDC_DTR_POLL_MS),
				K_MSEC(USB_CDC_DTR_POLL_MS));
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, uart_data_event);
EVENT_SUBSCRIBE_FINAL(MODULE, cdc_data_event);
