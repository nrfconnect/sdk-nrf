/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zboss_api.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>

#if defined ZB_NRF_TRACE

#include <zephyr/logging/log.h>

#define ZIGBEE_RX_BUF_LEN 16

LOG_MODULE_DECLARE(zboss_osif, CONFIG_ZBOSS_OSIF_LOG_LEVEL);

RING_BUF_DECLARE(logger_buf, CONFIG_ZBOSS_TRACE_LOGGER_BUFFER_SIZE);

static K_SEM_DEFINE(ringbuf_sem, 1, 1);

/* UART device used to print ZBOSS Trace messages. */
static const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(ncs_zboss_trace_uart));
static bool uart_dev_initialized;
static zb_callback_t char_handler;

static void handle_tx_ready_evt(const struct device *dev)
{
	zb_uint32_t data_len;
	zb_uint32_t data_taken;
	int ret_val;
	zb_uint8_t *data_ptr = NULL;

	ret_val = k_sem_take(&ringbuf_sem, K_NO_WAIT);

	if (ret_val == 0) {
		if (ring_buf_is_empty(&logger_buf)) {
			uart_irq_tx_disable(dev);
			k_sem_give(&ringbuf_sem);
			return;
		}
	} else {
		uart_irq_tx_disable(dev);
		return;
	}

	data_len = ring_buf_get_claim(&logger_buf,
				      &data_ptr,
				      CONFIG_ZBOSS_TRACE_LOGGER_BUFFER_SIZE);

	data_taken = uart_fifo_fill(dev, data_ptr, data_len);

	ret_val = ring_buf_get_finish(&logger_buf, data_taken);

	if (ret_val != 0) {
		LOG_ERR("%u exceeds valid bytes in the logger ring buffer", data_taken);
	}

	k_sem_give(&ringbuf_sem);
}

static void uart_rx_bytes(uint8_t *buf, size_t len)
{
	if (char_handler) {
		for (size_t i = 0; i < len; i++) {
			char_handler(buf[i]);
		}
	}
}

static void handle_rx_ready_evt(const struct device *dev)
{
	int recv_len = 0;
	uint8_t buffer[ZIGBEE_RX_BUF_LEN] = {0};

	recv_len = uart_fifo_read(dev, buffer, sizeof(buffer));

	while (recv_len != 0) {
		uart_rx_bytes(buffer, recv_len);

		recv_len = uart_fifo_read(dev, buffer, sizeof(buffer));
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

void zb_osif_serial_logger_put_bytes(const zb_uint8_t *buf, zb_short_t len)
{
	zb_uint8_t *buf_dest;
	zb_uint32_t allocated = 0;
	zb_uint32_t bytes_copied = 0;

	if (IS_ENABLED(CONFIG_ZBOSS_TRACE_LOG_LEVEL_OFF)) {
		return;
	}

	if (k_sem_take(&ringbuf_sem, K_FOREVER)) {
		LOG_ERR("Couldn't take semaphore, dropping %u bytes", len);
		return;
	}

	allocated = ring_buf_put_claim(&logger_buf, &buf_dest, len);

	while (allocated != 0) {
		ZB_MEMCPY(buf_dest, &buf[bytes_copied], allocated);
		ring_buf_put_finish(&logger_buf, allocated);
		bytes_copied += allocated;
		len -= allocated;
		allocated = ring_buf_put_claim(&logger_buf, &buf_dest, len);
	}

	if (len != 0) {
		LOG_DBG("Dropping %u bytes, ring buffer is full", len);
	}

	k_sem_give(&ringbuf_sem);
}

/* Is called when complete Trace message is put in the ring buffer.
 * Triggers sending buffered data through UART API.
 */
void zb_trace_msg_port_do(void)
{
	if (IS_ENABLED(CONFIG_ZBOSS_TRACE_LOG_LEVEL_OFF)) {
		return;
	}

	if (!uart_dev_initialized) {
		return;
	}

	uart_irq_tx_enable(uart_dev);
}

void zb_osif_serial_logger_init(void)
{
	/* Prevent multiple initiaizations as serial init function may be called more than once
	 * by ZBOSS stack.
	 */
	if (uart_dev_initialized) {
		return;
	}

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("ZBOSS Trace UART device not ready");
		return;
	}

	/* Enable and configure USB device if USB CDC ACM is used to log ZBOSS Traces. */
	if (IS_ENABLED(CONFIG_ZBOSS_TRACE_USB_CDC_LOGGING)) {
		int ret = usb_enable(NULL);

		if ((ret != 0) && (ret != -EALREADY)) {
			LOG_ERR("USB initialization failed - No UART device to log ZBOSS Traces");
			return;
		}

		/* Data Carrier Detect Modem - mark connection as established. */
		(void)uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_DCD, 1);
		/* Data Set Ready - the NCP SoC is ready to communicate. */
		(void)uart_line_ctrl_set(uart_dev, UART_LINE_CTRL_DSR, 1);
	}

	uart_irq_callback_set(uart_dev, interrupt_handler);

	/* Enable rx interrupts. */
	uart_irq_rx_enable(uart_dev);

	uart_dev_initialized = true;
}

void zb_osif_serial_logger_flush(void)
{
	if (!uart_dev_initialized) {
		return;
	}

	uart_irq_tx_enable(uart_dev);
	while (!ring_buf_is_empty(&logger_buf)) {
		k_sleep(K_MSEC(100));
	}
}

#if defined(CONFIG_ZB_NRF_TRACE_RX_ENABLE)
/* Function set UART RX callback function */
void zb_osif_serial_logger_set_uart_byte_received_cb(zb_callback_t cb)
{
	char_handler = cb;
}
#endif /* CONFIG_ZB_NRF_TRACE_RX_ENABLE */

#endif /* defined ZB_NRF_TRACE */
