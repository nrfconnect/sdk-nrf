/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zboss_api.h>
#include "zb_nrf_platform.h"
#include <zephyr/sys/ring_buffer.h>

static K_SEM_DEFINE(tx_done_sem, 1, 1);
static K_SEM_DEFINE(rx_done_sem, 1, 1);
static const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(ncs_zigbee_uart));
static bool is_sleeping;
static bool uart_initialized;

static zb_callback_t char_handler;
static serial_recv_data_cb_t rx_data_cb;
static serial_send_data_cb_t tx_data_cb;
static serial_send_data_cb_t tx_trx_data_cb;

#ifdef CONFIG_ZBOSS_TRACE_BINARY_NCP_TRANSPORT_LOGGING
static uint8_t uart_tx_buf_mem[CONFIG_ZIGBEE_UART_TX_BUF_LEN];
static size_t uart_tx_buf_size;
#endif /* CONFIG_ZBOSS_TRACE_BINARY_NCP_TRANSPORT_LOGGING */

static uint8_t *uart_tx_buf;
static uint8_t *uart_tx_buf_bak;
static volatile size_t uart_tx_buf_offset;
static volatile size_t uart_tx_buf_len;

static uint8_t uart_rx_buf_mem[CONFIG_ZIGBEE_UART_RX_BUF_LEN];
static struct ring_buf rx_ringbuf;

static uint8_t *uart_rx_buf;
static volatile size_t uart_rx_buf_offset;
static volatile size_t uart_rx_buf_len;

static void uart_tx_timeout(struct k_timer *dummy);
static void uart_rx_timeout(struct k_timer *dummy);

static K_TIMER_DEFINE(uart_tx_timer, uart_tx_timeout, NULL);
static K_TIMER_DEFINE(uart_rx_timer, uart_rx_timeout, NULL);

/**
 * Inform user about received data and unlock for the next reception.
 */
static void uart_rx_notify(zb_bufid_t bufid)
{
	uint8_t *rx_buf = uart_rx_buf;
	size_t rx_buf_len = uart_rx_buf_offset;

	ARG_UNUSED(bufid);

	uart_rx_buf_len = 0;
	uart_rx_buf_offset = 0;
	uart_rx_buf = NULL;
	k_timer_stop(&uart_rx_timer);
	k_sem_give(&rx_done_sem);

	if (rx_data_cb) {
		rx_data_cb(rx_buf, rx_buf_len);
	}
}

static void uart_rx_bytes(uint8_t *buf, size_t len)
{
	if (char_handler) {
		for (size_t i = 0; i < len; i++) {
			char_handler(buf[i]);
		}
	}
}

/**
 * Inform user about transmission timeout.
 */
static void uart_tx_timeout(struct k_timer *dummy)
{
	uart_tx_buf_offset = 0;
	uart_tx_buf_len = 0;
	uart_tx_buf = uart_tx_buf_bak;
	k_sem_give(&tx_done_sem);

	if (tx_trx_data_cb) {
		zigbee_schedule_callback(
			tx_trx_data_cb,
			SERIAL_SEND_TIMEOUT_EXPIRED);
		tx_trx_data_cb = NULL;
	}
}

/**
 * Inform user about reception timeout.
 */
static void uart_rx_timeout(struct k_timer *dummy)
{
	if (uart_rx_buf) {
		uart_rx_buf_len = 0;

		if (zigbee_schedule_callback(uart_rx_notify, 0)) {
			uart_rx_buf_offset = 0;
			uart_rx_buf = NULL;
			k_sem_give(&rx_done_sem);
		}
	}
}

static void handle_rx_ready_evt(const struct device *dev)
{
	int recv_len = 0;
	uint8_t buffer[CONFIG_ZIGBEE_UART_RX_BUF_LEN] = {0};

	/* Copy data to the user's buffer. */
	if (uart_rx_buf && (uart_rx_buf_offset < uart_rx_buf_len)) {
		uart_rx_buf_offset += ring_buf_get(
			&rx_ringbuf,
			&uart_rx_buf[uart_rx_buf_offset],
			uart_rx_buf_len - uart_rx_buf_offset);

		recv_len = uart_fifo_read(
			dev,
			&uart_rx_buf[uart_rx_buf_offset],
			uart_rx_buf_len - uart_rx_buf_offset);

		uart_rx_bytes(&uart_rx_buf[uart_rx_buf_offset], recv_len);
		uart_rx_buf_offset += recv_len;

		if (uart_rx_buf_offset == uart_rx_buf_len) {
			uart_rx_buf_len = 0;
			k_timer_stop(&uart_rx_timer);

			if (zigbee_schedule_callback(uart_rx_notify, 0)) {
				uart_rx_buf_offset = 0;
				uart_rx_buf = NULL;
				k_sem_give(&rx_done_sem);
			}
		} else {
			k_timer_start(
				&uart_rx_timer,
				K_MSEC(CONFIG_ZIGBEE_UART_PARTIAL_RX_TIMEOUT),
				K_NO_WAIT);
		}
	} else {
		recv_len = uart_fifo_read(dev, buffer, sizeof(buffer));
		uart_rx_bytes(buffer, recv_len);

		/* Store remaining bytes inside the ring buffer. */
		if (recv_len > ring_buf_space_get(&rx_ringbuf)) {
			uint8_t dummy_buffer[CONFIG_ZIGBEE_UART_RX_BUF_LEN];

			(void)ring_buf_get(&rx_ringbuf, dummy_buffer,
				recv_len - ring_buf_space_get(&rx_ringbuf));
		}

		(void)ring_buf_put(&rx_ringbuf, buffer, recv_len);
	}
}

static void handle_tx_ready_evt(const struct device *dev)
{
	if (uart_tx_buf_len <= uart_tx_buf_offset) {
		uart_irq_tx_disable(dev);
		return;
	}

	uart_tx_buf_offset += uart_fifo_fill(
		dev,
		&uart_tx_buf[uart_tx_buf_offset],
		uart_tx_buf_len - uart_tx_buf_offset);

	if (uart_tx_buf_len == uart_tx_buf_offset) {
		uart_tx_buf_offset = 0;
		uart_tx_buf_len = 0;
		uart_tx_buf = uart_tx_buf_bak;

		k_timer_stop(&uart_tx_timer);
		k_sem_give(&tx_done_sem);

		if (tx_trx_data_cb) {
			zigbee_schedule_callback(
				tx_trx_data_cb,
				SERIAL_SEND_SUCCESS);
			tx_trx_data_cb = NULL;
		}
	} else {
		k_timer_start(
			&uart_tx_timer,
			K_MSEC(CONFIG_ZIGBEE_UART_PARTIAL_TX_TIMEOUT),
			K_NO_WAIT);
	}
}

static void interrupt_handler(const struct device *dev, void *user_data)
{
	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			handle_rx_ready_evt(dev);
		}

		if (uart_irq_tx_ready(dev)) {
			handle_tx_ready_evt(dev);
		}
	}
}

void zb_osif_async_serial_init(void)
{
	if (uart_initialized) {
		return;
	}

	/*
	 * Reset all static variables in case of runtime init/uninit sequence.
	 */
	char_handler = NULL;
	rx_data_cb = NULL;
	tx_data_cb = NULL;
	tx_trx_data_cb = NULL;
	uart_tx_buf_len = 0;
	uart_tx_buf_offset = 0;
	uart_rx_buf = NULL;
	uart_rx_buf_len = 0;
	uart_rx_buf_offset = 0;

#ifdef CONFIG_ZBOSS_TRACE_BINARY_NCP_TRANSPORT_LOGGING
	uart_tx_buf_size = sizeof(uart_tx_buf_mem);
	uart_tx_buf = uart_tx_buf_mem;
	uart_tx_buf_bak = uart_tx_buf_mem;
#else
	uart_tx_buf = NULL;
	uart_tx_buf_bak = NULL;
#endif /* CONFIG_ZBOSS_TRACE_BINARY_NCP_TRANSPORT_LOGGING */

	if (!device_is_ready(uart_dev)) {
		return;
	}

	ring_buf_init(&rx_ringbuf, sizeof(uart_rx_buf_mem), uart_rx_buf_mem);
	uart_irq_callback_set(uart_dev, interrupt_handler);

	/* Enable rx interrupts. */
	uart_irq_rx_enable(uart_dev);

	uart_initialized = true;
}

void zb_osif_async_serial_sleep(void)
{
	if (uart_dev == NULL) {
		return;
	}

	is_sleeping = true;
	uart_irq_tx_disable(uart_dev);
	uart_irq_rx_disable(uart_dev);
}

void zb_osif_async_serial_wake_up(void)
{
	if (uart_dev == NULL) {
		return;
	}

	is_sleeping = false;

	/* Enable rx interrupts. */
	uart_irq_rx_enable(uart_dev);
}

void zb_osif_serial_recv_data(zb_uint8_t *buf, zb_ushort_t len)
{
	if (!rx_data_cb) {
		return;
	}

	if ((uart_dev == NULL) || (len == 0) || is_sleeping) {
		if (rx_data_cb) {
			rx_data_cb(NULL, 0);
		}
		return;
	}

	if (k_sem_take(&rx_done_sem,
		       K_MSEC(CONFIG_ZIGBEE_UART_RX_TIMEOUT))) {
		/* Ongoing asynchronous reception. */
		if (rx_data_cb) {
			rx_data_cb(NULL, 0);
		}
		return;
	}

	/*
	 * Flush already received data.
	 * Disable interrupt to block buffer reads from the interrupt handler.
	 */
	uart_irq_rx_disable(uart_dev);
	uart_rx_buf_offset = ring_buf_get(&rx_ringbuf, buf, len);
	uart_irq_rx_enable(uart_dev);

	if (uart_rx_buf_offset == len) {
		uart_rx_buf_offset = 0;
		k_sem_give(&rx_done_sem);
		rx_data_cb(buf, len);
		return;
	}

	/*
	 * Since the driver is kept in a continuous reception, it is enough to
	 * pass the buffer through a variable.
	 */
	uart_rx_buf_len = len;
	uart_rx_buf = buf;
}

void zb_osif_serial_set_cb_recv_data(serial_recv_data_cb_t cb)
{
	rx_data_cb = cb;
}

void zb_osif_serial_send_data(zb_uint8_t *buf, zb_ushort_t len)
{
	if ((uart_dev == NULL) || is_sleeping) {
		if (tx_data_cb) {
			tx_data_cb(SERIAL_SEND_ERROR);
		}
		return;
	}

	if (k_sem_take(&tx_done_sem, K_MSEC(CONFIG_ZIGBEE_UART_TX_TIMEOUT))) {
		/* Ongoing synchronous transmission. */
		if (tx_data_cb) {
			tx_data_cb(SERIAL_SEND_BUSY);
		}
		return;
	}

	uart_tx_buf_bak = uart_tx_buf;
	uart_tx_buf = buf;
	uart_tx_buf_len = len;
	uart_tx_buf_offset = 0;

	/* Pass the TX callback for a single (ongoing) transmission. */
	tx_trx_data_cb = tx_data_cb;

	/* Enable TX ready event. */
	uart_irq_tx_enable(uart_dev);
}

void zb_osif_serial_set_cb_send_data(serial_send_data_cb_t cb)
{
	tx_data_cb = cb;
}

void zb_osif_async_serial_flush(void)
{
	(void)k_sem_take(&tx_done_sem, K_FOREVER);
	k_sem_give(&tx_done_sem);
}

void zb_osif_async_serial_set_uart_byte_received_cb(zb_callback_t hnd)
{
	char_handler = hnd;
}

#ifdef CONFIG_ZBOSS_TRACE_BINARY_NCP_TRANSPORT_LOGGING
void zb_osif_set_user_io_buffer(zb_byte_array_t *buf_ptr, zb_ushort_t capacity)
{
	(void)k_sem_take(&tx_done_sem, K_FOREVER);

	uart_tx_buf = buf_ptr->ring_buf;
	uart_tx_buf_bak = uart_tx_buf;
	uart_tx_buf_size = capacity;

	k_sem_give(&tx_done_sem);
}

void zb_osif_async_serial_put_bytes(const zb_uint8_t *buf, zb_short_t len)
{
#if !(defined(ZB_HAVE_ASYNC_SERIAL) && \
	defined(CONFIG_ZBOSS_TRACE_LOG_LEVEL_OFF))

	if ((uart_dev == NULL) || is_sleeping) {
		return;
	}

	if (len > uart_tx_buf_size) {
		return;
	}

	/*
	 * Wait forever since there is no way to inform higher layer
	 * about TX busy state.
	 */
	(void)k_sem_take(&tx_done_sem, K_FOREVER);
	memcpy(uart_tx_buf, buf, len);

	uart_tx_buf_len = len;
	uart_tx_buf_offset = 0;

	/* Enable tx interrupts. */
	uart_irq_tx_enable(uart_dev);

#endif /* !(ZB_HAVE_ASYNC_SERIAL && CONFIG_ZBOSS_TRACE_LOG_LEVEL_OFF) */
}
#endif /* CONFIG_ZBOSS_TRACE_BINARY_NCP_TRANSPORT_LOGGING */
