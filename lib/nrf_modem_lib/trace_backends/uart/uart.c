/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <modem/trace_backend.h>
#include <zephyr/pm/device.h>

LOG_MODULE_REGISTER(modem_trace_backend, CONFIG_MODEM_TRACE_BACKEND_LOG_LEVEL);

#define UART_DEVICE_NODE DT_CHOSEN(nordic_modem_trace_uart)
#define TRACE_UART_BAUD_REQUIRED 1000000
#define CHUNK_SZ CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_CHUNK_SZ

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* Maximum time to wait for a UART transfer to complete before aborting. */
#define UART_TX_WAIT_TIME_MS 1000
/* Maximum UART transfer attempts. */
#define UART_TX_RETRIES 5

/* Semaphores used to synchronize UART transfers. */
static K_SEM_DEFINE(tx_sem, 0, 1);
static K_SEM_DEFINE(tx_done_sem, 0, 1);
/* Number of bytes that were transferred successfully in last transmission. */
static int tx_bytes;

/* Callback to notify the trace library when trace data is processed. */
static trace_backend_processed_cb trace_processed_callback;

static bool suspended;

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case UART_TX_DONE:
	case UART_TX_ABORTED:
		tx_bytes = evt->data.tx.len;
		k_sem_give(&tx_done_sem);
		break;
	case UART_RX_DISABLED:
		LOG_DBG("UART RX disabled");
		break;
	default:
		LOG_DBG("Unhandled UART event: %d", evt->type);
		break;
	}
}

int trace_backend_init(trace_backend_processed_cb trace_processed_cb)
{
	int err;
	struct uart_config uart_cfg;

	if (trace_processed_cb == NULL) {
		return -EFAULT;
	}

	trace_processed_callback = trace_processed_cb;

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	err = uart_config_get(uart_dev, &uart_cfg);
	if (err) {
		LOG_ERR("Failed to configure UART, err %d", err);
		return err;
	}

	if (uart_cfg.baudrate != TRACE_UART_BAUD_REQUIRED) {
		LOG_ERR("Trace UART baudrate is %d but %d is required",
			uart_cfg.baudrate, TRACE_UART_BAUD_REQUIRED);
		return -ENOTSUP;
	}

	err = uart_err_check(uart_dev);
	if (err) {
		LOG_ERR("UART check has failed, err %d", err);
		return -EIO;
	}

	err = uart_callback_set(uart_dev, uart_callback, NULL);
	if (err) {
		LOG_ERR("Cannot set callback, err %d", err);
		return -EFAULT;
	}

	k_sem_give(&tx_sem);

	return 0;
}

int trace_backend_deinit(void)
{
	return 0;
}

/* Returns the number of bytes written, or negative error. */
static int uart_send(const uint8_t *data, size_t len)
{
	int err;

	for (int i = 0; i < UART_TX_RETRIES; i++) {
		err = uart_tx(uart_dev, data, len,
			UART_TX_WAIT_TIME_MS * USEC_PER_MSEC);
		if (err) {
			LOG_ERR("UART TX failed, err %d", err);
			return err;
		}

		k_sem_take(&tx_done_sem, K_FOREVER);

		if (tx_bytes) {
			return tx_bytes;
		}
	}

	return 0;
}

int trace_backend_write(const void *data, size_t len)
{
	int err;
	int ret;

	uint8_t *buf = (uint8_t *)data;
	size_t remaining_bytes = len;

	if (suspended) {
		return -EPERM;
	}

	k_sem_take(&tx_sem, K_FOREVER);

	while (remaining_bytes) {
		/* Split buffer into smaller DMA-able chunks */
		size_t transfer_len = MIN(remaining_bytes, CHUNK_SZ);
		size_t idx = len - remaining_bytes;

		ret = uart_send(&buf[idx], transfer_len);
		if (ret < 0) {
			goto out;
		} else if (ret == 0) {
			break;
		}

		transfer_len = ret;

		/* Notify that we have processed the trace data, so that the memory can be freed. */
		err = trace_processed_callback(transfer_len);
		if (err) {
			ret = err;
			goto out;
		}

		remaining_bytes -= transfer_len;
	}

	ret = len - remaining_bytes;
out:
	k_sem_give(&tx_sem);

	if (ret == 0) {
		return -EAGAIN;
	}

	return ret;
}

int trace_backend_suspend(void)
{
#if CONFIG_PM_DEVICE
	int err;

	err = pm_device_action_run(uart_dev, PM_DEVICE_ACTION_SUSPEND);
	if (err) {
		LOG_ERR("Backend failed to enter suspended state, err %d\n", err);
	}

	suspended = true;
	LOG_DBG("Trace backend suspended");
#endif
	return 0;
}

int trace_backend_resume(void)
{
#if CONFIG_PM_DEVICE
	int err;

	err = pm_device_action_run(uart_dev, PM_DEVICE_ACTION_RESUME);
	if (err) {
		LOG_ERR("Backend failed to enter active state, err %d\n", err);
	}

	suspended = false;
	LOG_DBG("Trace backend resumed");
#endif

	return 0;
}

struct nrf_modem_lib_trace_backend trace_backend = {
	.init = trace_backend_init,
	.deinit = trace_backend_deinit,
	.write = trace_backend_write,
	.suspend = trace_backend_suspend,
	.resume = trace_backend_resume
};
