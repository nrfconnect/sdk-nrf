/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <kernel.h>
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

static K_SEM_DEFINE(tx_done_sem, 1, 1);
static K_SEM_DEFINE(rx_done_sem, 1, 1);
static K_SEM_DEFINE(rx_fill_buffer_sem, 1, 1);
static struct device *uart_dev;
static bool is_sleeping;

static zb_callback_t char_handler;
static serial_recv_data_cb_t rx_data_cb;
static serial_send_data_cb_t tx_data_cb;
static serial_send_data_cb_t tx_trx_data_cb;

static uint8_t *uart_tx_buf;
static size_t uart_tx_buf_len;
static uint8_t uart_tx_buf_mem[CONFIG_ZIGBEE_UART_TX_BUF_LEN];

static uint8_t *uart_rx_buf;
static volatile size_t uart_rx_buf_offset;
static volatile size_t uart_rx_buf_len;

/**
 * Add (CONFIG_ZIGBEE_UART_RX_CHUNK_LEN - 1) to the buffer size, so the driver
 * may write to the RX buffer directly, regardless the write pointer position.
 */
static uint8_t uart_rx_buf_mem[CONFIG_ZIGBEE_UART_RX_BUF_LEN +
			       CONFIG_ZIGBEE_UART_RX_CHUNK_LEN - 1];
static volatile size_t rx_write_i;
static volatile size_t rx_req_i;
static volatile size_t rx_read_i;

/**
 * Async: Push the buffer contents to the RX buffer.
 *
 * @param buf data buffer
 * @param len data length
 */
static void rx_buf_push(uint8_t *buf, size_t len)
{
	size_t cpy_len = 0;

	while (len) {
		/* Calculate the maximum length of the linear copy. */
		if (CONFIG_ZIGBEE_UART_RX_BUF_LEN - rx_write_i > len) {
			cpy_len = len;
		} else {
			cpy_len = CONFIG_ZIGBEE_UART_RX_BUF_LEN - rx_write_i;
		}

		memcpy(&uart_rx_buf_mem[rx_write_i], buf, cpy_len);

		/* Adjust the read pointer in case of buffer overflow. */
		if ((rx_read_i > rx_write_i) &&
		    (rx_read_i < rx_write_i + cpy_len)) {
			rx_read_i = (rx_write_i + cpy_len + 1) %
					CONFIG_ZIGBEE_UART_RX_BUF_LEN;
		}

		rx_write_i = (rx_write_i + cpy_len) %
				CONFIG_ZIGBEE_UART_RX_BUF_LEN;
		len -= cpy_len;
	}
}

/**
 * Async: Get the buffered data.
 *
 * @param buf destination data buffer
 * @param len destination data buffer length
 *
 * @return The number of bytes written to the destination buffer.
 */
static size_t rx_buf_pop(uint8_t *buf, size_t len)
{
	size_t copied = 0;
	size_t cpy_len = 0;

	while ((rx_read_i != rx_write_i) && len) {
		/* Calculate the maximum length of the linear copy. */
		if (rx_write_i > rx_read_i) {
			cpy_len = rx_write_i - rx_read_i;
		} else {
			cpy_len = CONFIG_ZIGBEE_UART_RX_BUF_LEN - rx_read_i;
		}

		/* Adjust the copy size to the input buffer length. */
		if (cpy_len > len) {
			cpy_len = len;
		}

		memcpy(&buf[copied], &uart_rx_buf_mem[rx_read_i], cpy_len);

		copied += cpy_len;
		len -= cpy_len;
		rx_read_i = (rx_read_i + cpy_len) %
				CONFIG_ZIGBEE_UART_RX_BUF_LEN;
	}

	return copied;
}

/**
 * Async: Increment the write pointer in RX buffer.
 *        This function is used to perform zero-copy data push, if the RX buffer
 *        was passed directly to the driver.
 *
 * @param len data length
 */
static void rx_buf_accept(size_t len)
{
	size_t cpy_len = 0;

	/* Calculate the amount of data written into readable area. */
	if (CONFIG_ZIGBEE_UART_RX_BUF_LEN - rx_write_i > len) {
		cpy_len = len;
	} else {
		cpy_len = CONFIG_ZIGBEE_UART_RX_BUF_LEN - rx_write_i;
	}

	/* Adjust the read pointer in case of buffer overflow. */
	if ((rx_read_i > rx_write_i) && (rx_read_i < rx_write_i + cpy_len)) {
		rx_read_i = (rx_write_i + cpy_len + 1) %
				CONFIG_ZIGBEE_UART_RX_BUF_LEN;
	}

	rx_write_i  = (rx_write_i + cpy_len) % CONFIG_ZIGBEE_UART_RX_BUF_LEN;
	len -= cpy_len;

	/*
	 * If the buffer was written in an unreadable area, copy the contents
	 * back to the beginning of the buffer using regular push operation.
	 */
	__ASSERT(len < CONFIG_ZIGBEE_UART_RX_CHUNK_LEN,
		"Memory corruption detected while incrementing buffer pointer");
	if (len > 0) {
		rx_buf_push(&uart_rx_buf_mem[CONFIG_ZIGBEE_UART_RX_BUF_LEN],
			    len);
	}
}

/**
 * Async: Get the expected pointer in the RX buffer for the reception of the
 *        next CONFIG_ZIGBEE_UART_RX_CHUNK_LEN bytes.
 *
 * @retval Pointer to the memory to write to.
 */
static uint8_t *rx_buf_get_next(void)
{
	size_t old_req_i = rx_req_i;

	rx_req_i = (rx_req_i + CONFIG_ZIGBEE_UART_RX_CHUNK_LEN) %
			CONFIG_ZIGBEE_UART_RX_BUF_LEN;
	return &uart_rx_buf_mem[old_req_i];
}


/**
 * Async: Inform user about received data and unlock for the next reception.
 */
static void uart_rx_notify(zb_bufid_t bufid)
{
	uint8_t *rx_buf = uart_rx_buf;
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
 * Async: Handle received bytes.
 *
 * @param buf data buffer
 * @param len data length.
 *
 * @return True if RX timeout detected and the RX operation should be restarted.
 */
static bool handle_rx_chunk(uint8_t *buf, size_t len)
{
	bool rx_timeout_occurred = false;

	/* Detect and handle the RX timeout event. */
	if ((buf >= uart_rx_buf_mem) &&
	    (buf < uart_rx_buf_mem + sizeof(uart_rx_buf_mem))) {
		/* Align write pointer. */
		rx_buf_accept(len);

		/* Copy data to the user's buffer. */
		if (uart_rx_buf_offset < uart_rx_buf_len) {
			uart_rx_buf_offset += rx_buf_pop(
				&uart_rx_buf[uart_rx_buf_offset],
				uart_rx_buf_len - uart_rx_buf_offset);
		}

		/* Detect RX timeout event. */
		if (len != CONFIG_ZIGBEE_UART_RX_CHUNK_LEN) {
			rx_timeout_occurred = true;
		}
	} else {
		/* Detect RX timeout event. */
		if ((uart_rx_buf_offset + len) != uart_rx_buf_len) {
			rx_timeout_occurred = true;
		}

		/*
		 * The received buffer is not one of RX chunks.
		 * The user buffer was filled in with data directly.
		 */
		uart_rx_buf_offset += len;
	}

	if (rx_timeout_occurred) {
		/* In case of RX timeout: return the currently buffered data. */
		uart_rx_buf_len = uart_rx_buf_offset;
		/*
		 * Disable RX in order to reset receiver and release user buffer
		 */
		uart_rx_disable(uart_dev);
	}

	return rx_timeout_occurred;
}

static void uart_evt_handler(struct device *dev, struct uart_event *evt,
			     void *user_data)
{
	ARG_UNUSED(dev);

	static bool restart_rx;
	static bool writing_user_buf;

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
		/* Sync: Pass received data through per-byte handler. */
		/* Async: Pass received data through per-buffer handler. */
		if (char_handler) {
			for (size_t i = evt->data.rx.offset;
			     i < evt->data.rx.offset + evt->data.rx.len;
			     i++) {
				char_handler(evt->data.rx.buf[i]);
			}
		}

		restart_rx = handle_rx_chunk(
			&evt->data.rx_buf.buf[evt->data.rx.offset],
			evt->data.rx.len);
		break;

	case UART_RX_BUF_REQUEST:
	{
		/*
		 * Sync: The continuous RX is implemented by receiving data
		 *       through chunks into static memory.
		 */
		/* Async: The continuous RX is implemented by passing chunk
		 *        buffers while the user buffer is unknown and filling
		 *        the user's buffer afterwards.
		 */
		/*
		 * Calculate the amount of data that can be written to the
		 * user's buffer, based on static RX buffer.
		 */
		size_t offset = uart_rx_buf_offset +
				CONFIG_ZIGBEE_UART_RX_CHUNK_LEN;
		if (uart_rx_buf &&
		    !writing_user_buf &&
		    (uart_rx_buf_len > offset)) {
			/*
			 * Async: The user provided RX buffer and the driver
			 *        has just received the first chunk of data,
			 *        which is already copied to user's buffer.
			 *        The next chunk is being received, so
			 *        skip additional chunk while passing the
			 *        user's buffer.
			 */
			uart_rx_buf_rsp(uart_dev,
				&(uart_rx_buf[offset]),
				uart_rx_buf_len - offset);
			writing_user_buf = true;
		} else {
			uart_rx_buf_rsp(uart_dev,
					rx_buf_get_next(),
					CONFIG_ZIGBEE_UART_RX_CHUNK_LEN);
		}
	}
	break;

	case UART_RX_BUF_RELEASED:
		if (uart_rx_buf && (uart_rx_buf_offset == uart_rx_buf_len)) {
			uart_rx_buf_len = 0;
			writing_user_buf = false;
			if (zigbee_schedule_callback(uart_rx_notify, 0)) {
				uart_rx_buf_offset = 0;
				uart_rx_buf = NULL;
				k_sem_give(&rx_fill_buffer_sem);
			}
		}
		break;

	case UART_RX_DISABLED:
		/*
		 * Reset the buffer request pointer, so the next buffer request
		 * will start writing data from write pointer.
		 */
		rx_req_i = rx_write_i;

		if (restart_rx && (!is_sleeping)) {
			/*
			 * Async: Reception was stopped due to RX timeout,
			 *        restart it automatically.
			 */
			restart_rx = false;
			(void)uart_rx_enable(
				uart_dev,
				rx_buf_get_next(),
				CONFIG_ZIGBEE_UART_RX_CHUNK_LEN,
				CONFIG_ZIGBEE_UART_PARTIAL_RX_TIMEOUT);
		} else {
			/*
			 * Reception finished and the next RX operation may
			 * be started through zb_osif_uart_wake_up() API.
			 */
			restart_rx = false;
			k_sem_give(&rx_done_sem);
		}
		break;

	case UART_RX_STOPPED:
		/* An error occurred during reception. Abort RX. */
		if (uart_rx_buf) {
			uart_rx_buf_len = 0;
			writing_user_buf = false;
			if (zigbee_schedule_callback(uart_rx_notify, 0)) {
				uart_rx_buf_offset = 0;
				uart_rx_buf = NULL;
				k_sem_give(&rx_fill_buffer_sem);
			}
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

	/*
	 * Reset all static variables in case of runtime init/uninit sequence.
	 */
	char_handler = NULL;
	rx_data_cb = NULL;
	tx_data_cb = NULL;
	tx_trx_data_cb = NULL;
	uart_tx_buf = uart_tx_buf_mem;
	uart_tx_buf_len = sizeof(uart_tx_buf_mem);
	uart_rx_buf = NULL;
	uart_rx_buf_len = 0;
	uart_rx_buf_offset = 0;
	rx_write_i = 0;
	rx_req_i = 0;
	rx_read_i = 0;

	uart_dev = device_get_binding(CONFIG_ZIGBEE_UART_DEVICE_NAME);
	if (uart_dev == NULL) {
		return;
	}

	if (uart_callback_set(uart_dev, uart_evt_handler, NULL)) {
		uart_dev = NULL;
		return;
	}

	(void)k_sem_take(&rx_done_sem, K_FOREVER);
	if (uart_rx_enable(uart_dev,
			   rx_buf_get_next(),
			   CONFIG_ZIGBEE_UART_RX_CHUNK_LEN,
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
	if (uart_rx_enable(uart_dev,
			   rx_buf_get_next(),
			   CONFIG_ZIGBEE_UART_RX_CHUNK_LEN,
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

	if ((uart_dev == NULL) || (len == 0)) {
		if (rx_data_cb) {
			rx_data_cb(NULL, 0);
		}
		return;
	}

	if (k_sem_take(&rx_fill_buffer_sem,
		       K_MSEC(CONFIG_ZIGBEE_UART_RX_TIMEOUT))) {
		/* Ongoing asynchronous reception. */
		if (rx_data_cb) {
			rx_data_cb(NULL, 0);
		}
		return;
	}

	/*
	 * Flush already received data.
	 */
	uart_rx_buf_offset = rx_buf_pop(buf, len);
	if (uart_rx_buf_offset == len) {
		uart_rx_buf_offset = 0;
		k_sem_give(&rx_fill_buffer_sem);
		rx_data_cb(buf, len);
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
