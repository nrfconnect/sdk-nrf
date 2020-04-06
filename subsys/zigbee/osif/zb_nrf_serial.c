/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <drivers/uart.h>
#include <zboss_api.h>
#include "zb_nrf_platform.h"

/*
 * Size of each of the first two received data chunks.
 *
 * Te reception through asynchronous API is performed in the following
 * block sizes:
 *   | RX_CHUNK | RX_CHUNK | (remaining) USER_RX_BUFFER |
 * As a result, the USER_RX_BUFFER has to accommodate at least two RX_CHUNKs.
 * Additionally, the size of a single RX_CHUNK is allowed.
 *
 * For a continuous reception, the following size ranges for USER_RX_BUFFER
 * are allowed:
 *  - RX_CHUNK
 *  - RX_CHUNK * 2 + n, where n is any non-negative integer value
 *
 * The implementation can handle shorter than expected transmissions
 * by handling RX_TIMEOUT UART event. This allows to receive transmissions of
 * any length, with the following exceptions:
 * - RX_CHUNK
 * - 2 * RX_CHUNK
 * In those two cases there is no RX_TIMEOUT event generated, thus the user
 * will not be informed about them immediately. The data will be passed to
 * the user with the next received bytes.
 */
#ifndef ZIGBEE_UART_RX_CHUNK_SIZE
#define ZIGBEE_UART_RX_CHUNK_SIZE 2
#endif /* ZIGBEE_UART_RX_CHUNK_SIZE */


static K_SEM_DEFINE(tx_done_sem, 1, 1);
static K_SEM_DEFINE(rx_done_sem, 1, 1);
static K_SEM_DEFINE(rx_fill_buffer_sem, 1, 1);
static struct device *uart_dev;
static bool is_sleeping;

static zb_callback_t char_handler;
static serial_recv_data_cb_t rx_data_cb;
static serial_send_data_cb_t tx_data_cb;
static serial_send_data_cb_t tx_trx_data_cb;

static u8_t *uart_tx_buf;
static size_t uart_tx_buf_len;
static u8_t uart_tx_buf_mem[CONFIG_ZIGBEE_UART_TX_BUF_LEN];

static u8_t *uart_rx_buf;
static size_t uart_rx_buf_offset;
static size_t uart_rx_buf_len;

static u8_t uart_rx_buf_chunk[2][ZIGBEE_UART_RX_CHUNK_SIZE];
static u8_t *uart_rx_buf_chunk_released;


/**
 * Async: Inform user about received data and unlock for the next reception.
 */
static void uart_rx_notify(zb_bufid_t bufid)
{
	u8_t *rx_buf = uart_rx_buf;
	size_t rx_buf_len = uart_rx_buf_offset;

	ARG_UNUSED(bufid);

	uart_rx_buf_len = 0;
	uart_rx_buf_offset = 0;
	uart_rx_buf = NULL;
	k_sem_give(&rx_fill_buffer_sem);

	if (rx_data_cb) {
		rx_data_cb(rx_buf, rx_buf_len);
	}
}

/**
 * Async: handle completion of buffer reception.
 *
 * @param rx_buf data buffer
 */
static void store_rx_buf(u8_t *rx_buf)
{
	if (uart_rx_buf_offset >= uart_rx_buf_len) {
		return;
	}

	/* The whole RX chunk/buffer received. */
	if ((rx_buf == uart_rx_buf_chunk[0]) ||
	    (rx_buf == uart_rx_buf_chunk[1])) {
		/*
		 * In async mode we have to consider both chunks, because both
		 * of them were passed before recv() API was called:
		 *  [0] - as the RX buffer. First bytes will go there.
		 *  [1] - as the next RX buffer, as the result of
		 *        uart_rx_enable() API call.
		 */
		memcpy(&uart_rx_buf[uart_rx_buf_offset],
		       rx_buf,
		       ZIGBEE_UART_RX_CHUNK_SIZE);
		uart_rx_buf_offset += ZIGBEE_UART_RX_CHUNK_SIZE;
	} else {
		/*
		 * The received buffer is not one of RX chunks.
		 * The user buffer is now fully filed in with data.
		 */
		uart_rx_buf_offset = uart_rx_buf_len;
	}
}

/**
 * Async: Pass partial RX buffer.
 *
 * @param buf data buffer
 * @param len data length.
 *
 * @return True if RX timeout detected.
 */
static bool handle_rx_chunk(u8_t *buf, size_t len)
{
	bool rx_timeout_occurred = false;

	/* Detect and handle the RX timeout event. */
	if ((buf == uart_rx_buf_chunk[0]) ||
	    (buf == uart_rx_buf_chunk[1])) {
		if (len != ZIGBEE_UART_RX_CHUNK_SIZE) {
			rx_timeout_occurred = true;
			memcpy(&uart_rx_buf[uart_rx_buf_offset], buf, len);
		}
	} else {
		if ((uart_rx_buf_offset + len) != uart_rx_buf_len) {
			rx_timeout_occurred = true;
			/*
			 * The driver was writing data directly to user's
			 * buffer, there is no need to do copying.
			 */
		}
	}

	if (rx_timeout_occurred) {
		uart_rx_buf_offset += len;
		uart_rx_buf_len = uart_rx_buf_offset;
		/*
		 * Disable RX in order to reset receiver and release user buffer
		 */
		uart_rx_disable(uart_dev);
	} else {
		/*
		 * The whole buffer was received - pass data to the user buffer.
		 */
		store_rx_buf(buf);
	}

	return rx_timeout_occurred;
}

static void uart_evt_handler(struct uart_event *evt, void *user_data)
{
	static bool restart_rx;
	static u8_t *blocked_buf_chunk;

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done_sem);

		if (tx_trx_data_cb) {
			zigbee_schedule_callback(
				tx_trx_data_cb,
				SERIAL_SEND_SUCCESS);
			tx_trx_data_cb = NULL;
		}
		break;

	case UART_TX_ABORTED:
		k_sem_give(&tx_done_sem);

		if (tx_trx_data_cb) {
			zigbee_schedule_callback(
				tx_trx_data_cb,
				SERIAL_SEND_TIMEOUT_EXPIRED);
			tx_trx_data_cb = NULL;
		}
		break;

	case UART_RX_RDY:
		/* New chunk of data received. */
		/* Sync: Pass received data through per-byte handler. */
		/* Async: Copy the data to the user's buffer or mark the
		 *        transfer as completed.
		 */
		if (char_handler) {
			for (size_t i = evt->data.rx.offset;
			     i < evt->data.rx.offset + evt->data.rx.len;
			     i++) {
				char_handler(evt->data.rx.buf[i]);
			}
		}

		if (uart_rx_buf) {
			restart_rx = handle_rx_chunk(evt->data.rx_buf.buf,
				       evt->data.rx.offset + evt->data.rx.len);
		}
		break;

	case UART_RX_BUF_REQUEST:
		/*
		 * Sync: The continuous RX is implemented by receiving data
		 *       through chunks into static memory.
		 */
		/* Async: The continuous RX is implemented by passing chunk
		 *        buffers while the user buffer is unknown and filling
		 *        the user's buffer afterwards.
		 */
		if (uart_rx_buf &&
		    !blocked_buf_chunk &&
		    uart_rx_buf_len > ZIGBEE_UART_RX_CHUNK_SIZE * 2) {
			/*
			 * Async: The user provided RX buffer and the driver
			 *        has just received the first chunk of data,
			 *        which is already copied to user's buffer.
			 *        The second chunk is being received, so
			 *        skip two chunks while passing the
			 *        user's buffer.
			 */
			uart_rx_buf_rsp(uart_dev,
			    &(uart_rx_buf[ZIGBEE_UART_RX_CHUNK_SIZE * 2]),
			    uart_rx_buf_len - ZIGBEE_UART_RX_CHUNK_SIZE * 2);

			/*
			 * Async: Remember the free RX chunk and restore it once
			 *        user's RX buffer will be filled with data.
			 */
			blocked_buf_chunk = uart_rx_buf_chunk_released;
		} else if (uart_rx_buf_chunk_released) {
			/* Sync: Pass the most recently released buffer. */
			uart_rx_buf_rsp(uart_dev,
					uart_rx_buf_chunk_released,
					ZIGBEE_UART_RX_CHUNK_SIZE);
		}

		/* The RX chunk was either remembered or used for reception. */
		uart_rx_buf_chunk_released = NULL;
		break;

	case UART_RX_BUF_RELEASED:
		if (uart_rx_buf) {
			if (uart_rx_buf_offset == uart_rx_buf_len) {
				uart_rx_buf_len = 0;
				zigbee_schedule_callback(uart_rx_notify, 0);
			}
		}

		/*
		 * Pass the consumed buffer through uart_rx_buf_chunk_released
		 * variable to keep RX chunks in a loop.
		 */
		if (!uart_rx_buf_chunk_released) {
			if ((evt->data.rx_buf.buf == uart_rx_buf_chunk[0]) ||
			    (evt->data.rx_buf.buf == uart_rx_buf_chunk[1])) {
				uart_rx_buf_chunk_released =
					evt->data.rx_buf.buf;
			} else if (blocked_buf_chunk) {
				uart_rx_buf_chunk_released = blocked_buf_chunk;
				blocked_buf_chunk = NULL;
			}
		}
		break;

	case UART_RX_DISABLED:
		if (restart_rx && (!is_sleeping)) {
			/*
			 * Async: Reception was stopped due to RX timeout,
			 *        restart it automatically.
			 */
			restart_rx = false;
			uart_rx_buf_chunk_released = uart_rx_buf_chunk[1];
			(void)uart_rx_enable(
				uart_dev,
				uart_rx_buf_chunk[0],
				ZIGBEE_UART_RX_CHUNK_SIZE,
				CONFIG_ZIGBEE_UART_PARTIAL_RX_TIMEOUT);
		} else {
			/*
			 * Reception finished and the next RX operation may
			 * be started.
			 */
			restart_rx = false;
			k_sem_give(&rx_done_sem);
		}
		break;

	case UART_RX_STOPPED:
		/* An error occurred during reception. Abort RX. */
		if (uart_rx_buf) {
			if (blocked_buf_chunk) {
				uart_rx_buf_chunk_released = blocked_buf_chunk;
				blocked_buf_chunk = NULL;
			}
			uart_rx_buf_len = 0;
			zigbee_schedule_callback(uart_rx_notify, 0);
		}
		break;
	default:
		break;
	}
}


void zb_osif_serial_init(void)
{
	if (uart_dev != NULL) {
		return;
	}

	char_handler = NULL;
	rx_data_cb = NULL;
	tx_data_cb = NULL;
	tx_trx_data_cb = NULL;
	uart_tx_buf = uart_tx_buf_mem;
	uart_tx_buf_len = sizeof(uart_tx_buf_mem);
	uart_rx_buf = NULL;
	uart_rx_buf_len = 0;
	uart_rx_buf_offset = 0;

	uart_dev = device_get_binding(CONFIG_ZIGBEE_UART_DEVICE_NAME);
	if (uart_dev == NULL) {
		return;
	}

	if (uart_callback_set(uart_dev, uart_evt_handler, NULL)) {
		uart_dev = NULL;
		return;
	}

	(void)k_sem_take(&rx_done_sem, K_FOREVER);
	uart_rx_buf_chunk_released = uart_rx_buf_chunk[1];
	if (uart_rx_enable(uart_dev,
			   uart_rx_buf_chunk[0],
			   ZIGBEE_UART_RX_CHUNK_SIZE,
			   CONFIG_ZIGBEE_UART_PARTIAL_RX_TIMEOUT)) {
		k_sem_give(&rx_done_sem);
		uart_dev = NULL;
		return;
	}
}

void zb_osif_set_uart_byte_received_cb(zb_callback_t hnd)
{
	char_handler = hnd;
}

void zb_osif_set_user_io_buffer(zb_byte_array_t *buf_ptr, zb_ushort_t capacity)
{
	(void)k_sem_take(&tx_done_sem, K_FOREVER);

	uart_tx_buf = buf_ptr->ring_buf;
	uart_tx_buf_len = capacity;

	k_sem_give(&tx_done_sem);
}

void zb_osif_uart_sleep(void)
{
	if (uart_dev == NULL) {
		return;
	}

	is_sleeping = true;
	(void)uart_tx_abort(uart_dev);
	(void)uart_rx_disable(uart_dev);
}

void zb_osif_uart_wake_up(void)
{
	if (uart_dev == NULL) {
		return;
	}

	(void)k_sem_take(&rx_done_sem, K_FOREVER);
	is_sleeping = false;
	uart_rx_buf_chunk_released = uart_rx_buf_chunk[1];
	if (uart_rx_enable(uart_dev,
			   uart_rx_buf_chunk[0],
			   ZIGBEE_UART_RX_CHUNK_SIZE,
			   CONFIG_ZIGBEE_UART_PARTIAL_RX_TIMEOUT)) {
		k_sem_give(&rx_done_sem);
		uart_dev = NULL;
	}
}

void zb_osif_serial_put_bytes(zb_uint8_t *buf, zb_short_t len)
{
	if (uart_dev == NULL) {
		return;
	}

	if (len > uart_tx_buf_len) {
		return;
	}

	/*
	 * Wait forever since there is no way to inform higher layer about
	 * TX busy state.
	 */
	(void)k_sem_take(&tx_done_sem, K_FOREVER);
	memcpy(uart_tx_buf, buf, len);

	if (uart_tx(uart_dev, uart_tx_buf, len, SYS_FOREVER_MS)) {
		/* TX busy due to async operation. */
		k_sem_give(&tx_done_sem);
		return;
	}
}

void zb_osif_serial_recv_data(zb_uint8_t *buf, zb_ushort_t len)
{
	if (!rx_data_cb) {
		return;
	}

	if ((uart_dev == NULL) ||
	    ((len < 2 * ZIGBEE_UART_RX_CHUNK_SIZE) &&
	     (len != ZIGBEE_UART_RX_CHUNK_SIZE))) {
		if (rx_data_cb) {
			rx_data_cb(NULL, 0);
		}
		return;
	}

	if (k_sem_take(&rx_fill_buffer_sem,
		       K_MSEC(CONFIG_ZIGBEE_UART_RX_TIMEOUT))) {
		/* Ongoing asynchronous reception. */
		if (rx_data_cb) {
			k_sem_give(&rx_fill_buffer_sem);
			rx_data_cb(NULL, 0);
		}
		return;
	}

	/*
	 * Since the driver is kept in a continuous reception, it is enough to
	 * pass the buffer through variable.
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
	if (uart_dev == NULL) {
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

	if (uart_tx(uart_dev, buf, len, CONFIG_ZIGBEE_UART_TX_TIMEOUT)) {
		k_sem_give(&tx_done_sem);
		if (tx_data_cb) {
			tx_data_cb(SERIAL_SEND_BUSY);
		}
		return;
	}

	/* Pass the TX callback for a single (ongoing) tranmission. */
	tx_trx_data_cb = tx_data_cb;
}

void zb_osif_serial_set_cb_send_data(serial_send_data_cb_t cb)
{
	tx_data_cb = cb;
}

void zb_osif_serial_flush(void)
{
	(void)k_sem_take(&tx_done_sem, K_FOREVER);
	k_sem_give(&tx_done_sem);
}
