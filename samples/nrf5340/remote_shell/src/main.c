/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/usb/usb_device.h>

#include "shell_ipc_host.h"

#include <zephyr/logging/log.h>

#define HOST_WAIT_TIME 1000000

LOG_MODULE_REGISTER(remote_shell);

RING_BUF_DECLARE(rx_ringbuf, CONFIG_REMOTE_SHELL_RX_RING_BUFFER_SIZE);
RING_BUF_DECLARE(tx_ringbuf, CONFIG_REMOTE_SHELL_TX_RING_BUFFER_SIZE);

K_SEM_DEFINE(shell_ipc_write_sem, 0, 1);

static const struct uart_irq_context {
	struct ring_buf *rx_buf;
	struct ring_buf *tx_buf;
	struct k_sem *write_sem;
} uart_context = {
	.rx_buf = &rx_ringbuf,
	.tx_buf = &tx_ringbuf,
	.write_sem = &shell_ipc_write_sem
};

static void uart_dtr_wait(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_UART_LINE_CTRL)) {
		int dtr, err;

		while (true) {
			err = uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
			if (err == -ENOSYS || err == -ENOTSUP) {
				break;
			}
			if (dtr) {
				break;
			}
			/* Give CPU resources to low priority threads. */
			k_sleep(K_MSEC(100));
		}
	}
}

static void ipc_host_receive(const uint8_t *data, size_t len, void *context)
{
	int recv_len;
	const struct device *uart_dev = context;

	if (!data || (len == 0)) {
		return;
	}

	recv_len = ring_buf_put(&tx_ringbuf, data, len);
	if (recv_len < len) {
		LOG_INF("TX ring buffer full. Dropping %d bytes", len - recv_len);
	}

	uart_irq_tx_enable(uart_dev);
}

static void uart_tx_procces(const struct device *dev, const struct uart_irq_context *uart_ctrl)
{
	int err;
	int send_len = 0;
	uint8_t *data;

	send_len  = ring_buf_get_claim(uart_ctrl->tx_buf, &data, uart_ctrl->tx_buf->size);
	if (send_len) {
		if (IS_ENABLED(CONFIG_UART_LINE_CTRL)) {
			uart_dtr_wait(dev);
		}

		send_len = uart_fifo_fill(dev, data, send_len);
		err = ring_buf_get_finish(uart_ctrl->tx_buf, send_len);
		__ASSERT_NO_MSG(err == 0);
	} else {
		uart_irq_tx_disable(dev);
	}
}

static void uart_rx_process(const struct device *dev, const struct uart_irq_context *uart_ctrl)
{
	int recv_len = 0;
	uint8_t *data;

	recv_len = ring_buf_put_claim(uart_ctrl->rx_buf, &data, uart_ctrl->rx_buf->size);
	if (recv_len) {
		recv_len = uart_fifo_read(dev, data, recv_len);
		if (recv_len < 0) {
			LOG_ERR("Failed to read UART FIFO, err %d", recv_len);
			recv_len = 0;
		} else {
			ring_buf_put_finish(uart_ctrl->rx_buf, recv_len);
		}

		if (recv_len) {
			k_sem_give(uart_ctrl->write_sem);
		}
	}
}

static void uart_irq_handler(const struct device *dev, void *user_data)
{
	const struct uart_irq_context *uart_ctrl = user_data;

	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			uart_rx_process(dev, uart_ctrl);
		}

		if (uart_irq_tx_ready(dev)) {
			uart_tx_procces(dev, uart_ctrl);
		}
	}
}

int main(void)
{
	int err;
	const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(ncs_remote_shell_uart));
	uint32_t baudrate = 0;

	if (!device_is_ready(dev)) {
		LOG_ERR("Device not ready");
		return 0;
	}

	if (IS_ENABLED(CONFIG_USB_DEVICE_STACK)) {
		err = usb_enable(NULL);
		if (err != 0) {
			LOG_ERR("Failed to enable USB, err %d", err);
			return 0;
		}
	}

	if (IS_ENABLED(CONFIG_UART_LINE_CTRL)) {
		LOG_INF("Wait for DTR");

		uart_dtr_wait(dev);

		LOG_INF("DTR set");

		/* They are optional, we use them to test the interrupt endpoint */
		err = uart_line_ctrl_set(dev, UART_LINE_CTRL_DCD, 1);
		if (err) {
			LOG_WRN("Failed to set DCD, err %d", err);
		}

		err = uart_line_ctrl_set(dev, UART_LINE_CTRL_DSR, 1);
		if (err) {
			LOG_WRN("Failed to set DSR, err %d", err);
		}

		/* Wait 1 sec for the host to do all settings */
		k_busy_wait(HOST_WAIT_TIME);

		err = uart_line_ctrl_get(dev, UART_LINE_CTRL_BAUD_RATE, &baudrate);
		if (err) {
			LOG_WRN("Failed to get baudrate, err %d", err);
		} else {
			LOG_INF("Baudrate detected: %d", baudrate);
		}
	}

	uart_irq_callback_user_data_set(dev, uart_irq_handler, (void *)&uart_context);

	/* Enable rx interrupts */
	uart_irq_rx_enable(dev);

	err = shell_ipc_host_init(ipc_host_receive, (void *)dev);
	if (err) {
		LOG_ERR("Shell IPC host initialization failed, err %d", err);
		return 0;
	}

	for (;;) {
		uint8_t *data;
		uint32_t data_size;

		k_sem_take(&shell_ipc_write_sem, K_FOREVER);

		data_size = ring_buf_get_claim(&rx_ringbuf, &data, rx_ringbuf.size);

		if (data_size) {
			data_size = shell_ipc_host_write(data, data_size);
			if (data_size >= 0) {
				LOG_DBG("Sent %d bytes", data_size);
				ring_buf_get_finish(&rx_ringbuf, data_size);
			} else {
				LOG_INF("Failed to send %d bytes (%d error)",
					data_size, data_size);
			}
		}
	}
}
