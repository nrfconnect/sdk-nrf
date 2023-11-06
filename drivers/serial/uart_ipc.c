/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>

#define DT_DRV_COMPAT nordic_nrf_ipc_uart

#define BIND_TIMEOUT     CONFIG_IPC_UART_BIND_TIMEOUT_MS
#define IPC_RESEND_DELAY CONFIG_IPC_UART_RESEND_DELAY

LOG_MODULE_REGISTER(ipc_uart, CONFIG_IPC_UART_LOG_LEVEL);

struct uart_ipc_config {
	const struct device *ipc;
	const char *ept_name;
};

struct uart_ipc_data {
	struct ipc_ept ept;
	struct ipc_ept_cfg ept_cfg;
	struct k_event ept_bond;
	struct k_work_delayable ipc_send_work;
	struct ring_buf *tx_ringbuf;
	struct ring_buf *rx_ringbuf;
};

static const struct uart_ipc_config *get_config(const struct device *dev)
{
	return dev->config;
}

static struct uart_ipc_data *get_data(const struct device *dev)
{
	return dev->data;
};

static void ipc_send_work_handler(struct k_work *work)
{
	int ret;
	uint32_t data_size;
	uint8_t *data;
	struct uart_ipc_data *dev_data;

	dev_data = CONTAINER_OF(work, struct uart_ipc_data, ipc_send_work);

	data_size = ring_buf_get_claim(dev_data->tx_ringbuf, &data,
				       ring_buf_size_get(dev_data->tx_ringbuf));
	if (data_size == 0) {
		/* Nothing to send. */
		return;
	}

	ret = ipc_service_send(&dev_data->ept, data, data_size);
	if (ret == -ENOMEM) {
		k_work_reschedule(&dev_data->ipc_send_work, K_MSEC(IPC_RESEND_DELAY));
	} else if (ret < 0) {
		__ASSERT(false, "Failed to send data over ipc, ret: %d", ret);
		ret = 0;
	} else {
		/* Do nothing */
	}

	ret = ring_buf_get_finish(dev_data->tx_ringbuf, ret);
	if (ret) {
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
	}
}

static void ipc_ept_bound(void *priv)
{
	const struct device *dev = priv;
	struct uart_ipc_data *data = get_data(dev);

	k_event_set(&data->ept_bond, 0x01);
}

static void ipc_ept_data_received(const void *data, size_t len, void *priv)
{
	int ret;
	const struct device *dev = priv;
	struct uart_ipc_data *dev_data = get_data(dev);

	ret = ring_buf_put(dev_data->rx_ringbuf, data, len);
	if (ret != len) {
		LOG_INF("RX ring buffer full. Dropping %d bytes", len - ret);
	}
}

static void ipc_ept_error(const char *message, void *priv)
{
	LOG_ERR("UART IPC error: %s", message);
	__ASSERT_NO_MSG(false);
}

int ipc_uart_poll_in(const struct device *dev, unsigned char *p_char)
{
	int ret;
	struct uart_ipc_data *data = get_data(dev);

	ret = ring_buf_get(data->rx_ringbuf, (uint8_t *)p_char, sizeof(*p_char));
	if (ret == sizeof(*p_char)) {
		LOG_DBG("Received character through IPC: %c", *p_char);
	} else if (ret == 0) {
		LOG_DBG("Receive buffer is empty");
	} else {
		/* Should not happen. */
		__ASSERT_NO_MSG(false);
	}

	return (ret == 0) ? (-1) : 0;
}

void ipc_uart_poll_out(const struct device *dev, unsigned char out_char)
{
	int ret;
	struct uart_ipc_data *data = get_data(dev);

	do {
		ret = ring_buf_put(data->tx_ringbuf, (uint8_t *)(&out_char), sizeof(out_char));
		/* We do not want to busy wait here in case when buffer is full. */
		if (ret < sizeof(out_char)) {
			k_msleep(1);
		}
	} while (ret < sizeof(out_char));

	/* Should not happen. */
	if (ret > sizeof(out_char)) {
		__ASSERT(false, "Invalid bytes count returned from buffer");
	}

	k_work_schedule(&data->ipc_send_work, K_NO_WAIT);
}

int ipc_uart_init(const struct device *dev)
{
	int err;
	uint32_t events;
	const struct uart_ipc_config *config = get_config(dev);
	struct uart_ipc_data *data = get_data(dev);

	k_work_init_delayable(&data->ipc_send_work, ipc_send_work_handler);

	err = ipc_service_open_instance(config->ipc);
	if (err) {
		LOG_ERR("Failed to open IPC instance: %d", err);
		return err;
	} else if (err == -EALREADY) {
		LOG_INF("IPC service instance already opened");
	} else {
		LOG_INF("IPC service instance opened");
	}

	data->ept_cfg.name = config->ept_name;
	data->ept_cfg.priv = (void *)dev;
	data->ept_cfg.cb.bound = ipc_ept_bound;
	data->ept_cfg.cb.received = ipc_ept_data_received;
	data->ept_cfg.cb.error = ipc_ept_error;

	k_event_init(&data->ept_bond);

	err = ipc_service_register_endpoint(config->ipc, &data->ept, &data->ept_cfg);
	if (err) {
		LOG_ERR("Failed to register endpoint (err %d)", err);
		return err;
	}

	events = k_event_wait(&data->ept_bond, 0x01, false, K_MSEC(BIND_TIMEOUT));
	if (events == 0) {
		LOG_ERR("IPC endpoint bond timeout");
		return -EPIPE;
	}

	LOG_INF("IPC endpoint bonded");
	LOG_INF("UART ipc driver initialized for device: %s", dev->name);

	return 0;
}

static const struct uart_driver_api uart_ipc_api = {
	.poll_in = ipc_uart_poll_in,
	.poll_out = ipc_uart_poll_out
};

#define UART_IPC_DEVICE(idx)                                                                    \
	static const struct uart_ipc_config _CONCAT(uart_ipc_config_, idx) = {                  \
		.ipc = DEVICE_DT_GET(DT_INST_PHANDLE(idx, ipc)),                                \
		.ept_name = DT_INST_PROP(idx, ept_name)                                         \
	};                                                                                      \
                                                                                                \
	RING_BUF_DECLARE(ipc_uart_tx_buf_##idx, CONFIG_IPC_UART_TX_RING_BUFFER_SIZE);           \
	RING_BUF_DECLARE(ipc_uart_rx_buf_##idx, CONFIG_IPC_UART_RX_RING_BUFFER_SIZE);           \
                                                                                                \
	static struct uart_ipc_data _CONCAT(uart_ipc_data_, idx) = {                            \
		.tx_ringbuf = &_CONCAT(ipc_uart_tx_buf_, idx),                                  \
		.rx_ringbuf = &_CONCAT(ipc_uart_rx_buf_, idx)                                   \
	};                                                                                      \
                                                                                                \
	DEVICE_DT_INST_DEFINE(idx, ipc_uart_init, NULL,                                         \
			      &_CONCAT(uart_ipc_data_, idx), &_CONCAT(uart_ipc_config_, idx),   \
			      POST_KERNEL, CONFIG_IPC_UART_INIT_PRIORITY, &uart_ipc_api);

DT_INST_FOREACH_STATUS_OKAY(UART_IPC_DEVICE)
