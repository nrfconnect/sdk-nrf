/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/drivers/uart.h>
#include <hal/nrf_uarte.h>
#include <hal/nrf_gpio.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/pm/device.h>
#include <pm_config.h>
#include "slm_settings.h"
#include "slm_uart_handler.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(slm_uart_handler, CONFIG_SLM_LOG_LEVEL);

#define UART_RX_TIMEOUT_US		2000
#define UART_ERROR_DELAY_MS		500

#define SLM_SYNC_STR	"Ready\r\n"

static const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(ncs_slm_uart));

static struct k_work_delayable rx_process_work;

struct rx_buf_t {
	atomic_t ref_counter;
	size_t len;
	uint8_t buf[CONFIG_SLM_UART_RX_BUF_SIZE];
};

#define UART_SLAB_BLOCK_SIZE sizeof(struct rx_buf_t)
#define UART_SLAB_BLOCK_COUNT CONFIG_SLM_UART_RX_BUF_COUNT
#define UART_SLAB_ALIGNMENT 4
BUILD_ASSERT((sizeof(struct rx_buf_t) % UART_SLAB_ALIGNMENT) == 0);
K_MEM_SLAB_DEFINE(rx_slab, UART_SLAB_BLOCK_SIZE, UART_SLAB_BLOCK_COUNT, UART_SLAB_ALIGNMENT);

/* 4 messages for 512 bytes, 32 messages for 4096 bytes. */
#define UART_RX_EVENT_COUNT ((CONFIG_SLM_UART_RX_BUF_COUNT * CONFIG_SLM_UART_RX_BUF_SIZE) / 128)
#define UART_RX_EVENT_COUNT_FOR_BUF (UART_RX_EVENT_COUNT / CONFIG_SLM_UART_RX_BUF_COUNT)
struct rx_event_t {
	uint8_t *buf;
	size_t len;
};
K_MSGQ_DEFINE(rx_event_queue, sizeof(struct rx_event_t), UART_RX_EVENT_COUNT, 4);

RING_BUF_DECLARE(tx_buf, CONFIG_SLM_UART_TX_BUF_SIZE);
K_MUTEX_DEFINE(mutex_tx_put); /* Protects the tx_buf from multiple writes. */

enum uart_recovery_state {
	RECOVERY_IDLE,
	RECOVERY_ONGOING,
	RECOVERY_DISABLED
};
static atomic_t recovery_state;

K_SEM_DEFINE(tx_done_sem, 0, 1);

slm_uart_rx_callback_t rx_callback_t;

/* global functions defined in different files */
int indicate_start(void);

static inline struct rx_buf_t *block_start_get(uint8_t *buf)
{
	size_t block_num;

	/* blocks are fixed size units from a continuous memory slab: */
	/* round down to the closest unit size to find beginning of block. */

	block_num =
		(((size_t)buf - (size_t)rx_slab.buffer) / UART_SLAB_BLOCK_SIZE);

	return (struct rx_buf_t *) &rx_slab.buffer[block_num * UART_SLAB_BLOCK_SIZE];
}

static struct rx_buf_t *rx_buf_alloc(void)
{
	struct rx_buf_t *buf;
	int err;

	/* Async UART driver returns pointers to received data as */
	/* offsets from beginning of RX buffer block. */
	/* This code uses a reference counter to keep track of the number of */
	/* references within a single RX buffer block */

	err = k_mem_slab_alloc(&rx_slab, (void **) &buf, K_NO_WAIT);
	if (err) {
		return NULL;
	}

	atomic_set(&buf->ref_counter, 1);

	return buf;
}

static void rx_buf_ref(void *buf)
{
	atomic_inc(&(block_start_get(buf)->ref_counter));
}

static void rx_buf_unref(void *buf)
{
	struct rx_buf_t *uart_buf = block_start_get(buf);
	atomic_t ref_counter = atomic_dec(&uart_buf->ref_counter);

	/* ref_counter is the uart_buf->ref_counter value prior to decrement */
	if (ref_counter == 1) {
		k_mem_slab_free(&rx_slab, (void **)&uart_buf);
	}
}

static int rx_enable(void)
{
	struct rx_buf_t *buf;
	int ret;

	buf = rx_buf_alloc();
	if (!buf) {
		LOG_ERR("UART RX failed to allocate buffer");
		return -ENOMEM;
	}

	ret = uart_rx_enable(uart_dev, buf->buf, sizeof(buf->buf), UART_RX_TIMEOUT_US);
	if (ret) {
		LOG_ERR("UART RX enable failed: %d", ret);
		return ret;
	}

	return 0;
}

static void rx_disable(void)
{
	int err;

	/* Wait for possible rx_enable to complete. */
	if (atomic_set(&recovery_state, RECOVERY_DISABLED) == RECOVERY_ONGOING) {
		k_sleep(K_MSEC(10));
	}

	err = uart_rx_disable(uart_dev);
	if (err) {
		LOG_ERR("UART RX disable failed: %d", err);
		return;
	}

	/* Wait for disable to complete. */
	k_sleep(K_MSEC(100));
}

static void rx_recovery(void)
{
	int err;

	if (atomic_get(&recovery_state) != RECOVERY_ONGOING) {
		return;
	}

	err = rx_enable();
	if (err) {
		k_work_schedule(&rx_process_work, K_MSEC(UART_RX_MARGIN_MS));
		return;
	}

	if (atomic_cas(&recovery_state, RECOVERY_ONGOING, RECOVERY_IDLE)) {
		LOG_INF("UART RX enabled");
	}
}

static void rx_process(struct k_work *work)
{
	struct rx_event_t rx_event;

	while (k_msgq_get(&rx_event_queue, &rx_event, K_NO_WAIT) == 0) {
		if (rx_callback_t) {
			rx_callback_t(rx_event.buf, rx_event.len);
		}
		rx_buf_unref(rx_event.buf);
	}

	rx_recovery();
}

static int tx_start(void)
{
	uint8_t *buf;
	size_t ret;
	int err;
	enum pm_device_state state = PM_DEVICE_STATE_OFF;

	(void)pm_device_state_get(uart_dev, &state);
	if (state != PM_DEVICE_STATE_ACTIVE) {
		(void)indicate_start();
		return -ENODEV;
	}

	ret = ring_buf_get_claim(&tx_buf, &buf, ring_buf_capacity_get(&tx_buf));
	err = uart_tx(uart_dev, buf, ret, SYS_FOREVER_US);
	if (err) {
		LOG_ERR("UART TX error: %d", err);
		(void)ring_buf_get_finish(&tx_buf, 0);
		return err;
	}

	return 0;
}

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	struct rx_buf_t *buf;
	struct rx_event_t rx_event;
	int err;

	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case UART_TX_DONE:
		err = ring_buf_get_finish(&tx_buf, evt->data.tx.len);
		if (err) {
			LOG_ERR("UART_TX_DONE failure: %d", err);
		}
		if (ring_buf_is_empty(&tx_buf) == false) {
			(void)tx_start();
		} else {
			k_sem_give(&tx_done_sem);
		}
		break;
	case UART_TX_ABORTED:
		err = ring_buf_get_finish(&tx_buf, evt->data.tx.len);
		if (err) {
			LOG_ERR("UART_TX_ABORTED failure: %d", err);
		}
		if (ring_buf_is_empty(&tx_buf) == false) {
			(void)tx_start();
		} else {
			k_sem_give(&tx_done_sem);
		}
		break;
	case UART_RX_RDY:
		rx_buf_ref(evt->data.rx.buf);
		rx_event.buf = &evt->data.rx.buf[evt->data.rx.offset];
		rx_event.len = evt->data.rx.len;
		err = k_msgq_put(&rx_event_queue, &rx_event, K_NO_WAIT);
		if (err) {
			LOG_ERR("UART_RX_RDY failure: %d, dropped: %d", err, evt->data.rx.len);
			rx_buf_unref(evt->data.rx.buf);
			break;
		}
		(void)k_work_submit((struct k_work *)&rx_process_work);
		break;
	case UART_RX_BUF_REQUEST:
		if (k_msgq_num_free_get(&rx_event_queue) < UART_RX_EVENT_COUNT_FOR_BUF) {
			LOG_INF("Disabling UART RX: No event space.");
			break;
		}
		buf = rx_buf_alloc();
		if (!buf) {
			LOG_INF("Disabling UART RX: No free buffers.");
			break;
		}
		err = uart_rx_buf_rsp(uart_dev, buf->buf, sizeof(buf->buf));
		if (err) {
			LOG_WRN("Disabling UART RX: %d", err);
			rx_buf_unref(buf);
		}
		break;
	case UART_RX_BUF_RELEASED:
		if (evt->data.rx_buf.buf) {
			rx_buf_unref(evt->data.rx_buf.buf);
		}
		break;
	case UART_RX_STOPPED:
		LOG_WRN("UART_RX_STOPPED (%d)", evt->data.rx_stop.reason);
		break;
	case UART_RX_DISABLED:
		LOG_DBG("UART_RX_DISABLED");
		if (atomic_cas(&recovery_state, RECOVERY_IDLE, RECOVERY_ONGOING)) {
			k_work_submit((struct k_work *)&rx_process_work);
		}
		break;
	default:
		break;
	}
}

int slm_uart_power_on(void)
{
	int err;

	err = pm_device_action_run(uart_dev, PM_DEVICE_ACTION_RESUME);
	if (err && err != -EALREADY) {
		LOG_ERR("Can't resume UART: %d", err);
		return err;
	}

	k_sleep(K_MSEC(100));

	(void)atomic_set(&recovery_state, RECOVERY_IDLE);
	err = rx_enable();
	if (err) {
		return err;
	}

	k_sem_give(&tx_done_sem);
	(void)tx_start();

	return 0;
}

int slm_uart_power_off(void)
{
	int err;

	rx_disable();
	err = pm_device_action_run(uart_dev, PM_DEVICE_ACTION_SUSPEND);
	if (err && err != -EALREADY) {
		LOG_ERR("Can't suspend UART: %d", err);
	}

	/* Write sync str to buffer, so it is send first when we power UART.*/
	(void)slm_uart_tx_write(SLM_SYNC_STR, sizeof(SLM_SYNC_STR)-1, true);

	return err;
}

int slm_uart_configure(void)
{
	int err;

	LOG_DBG("Set UART baudrate to: %d", slm_uart.baudrate);

	err = uart_configure(uart_dev, &slm_uart);
	if (err != 0) {
		LOG_ERR("uart_configure: %d", err);
	}

	return err;
}

bool slm_uart_can_context_send(const uint8_t *data, size_t len)
{
	if (!k_is_in_isr()) {
		return true;
	}
	if (len >= 2 && data[0] == '\r' && data[1] == '\n') {
		/* Make so that the message is on the same line as the log. */
		data += 2;
		len -= 2;
	}
	LOG_ERR("FIXME: Attempt to send UART message in ISR: %.*s", len, data);
	return false;
}

/* Write the data to tx_buffer and trigger sending. */
int slm_uart_tx_write(const uint8_t *data, size_t len, bool print_full_debug)
{
	size_t ret;
	size_t sent = 0;
	int err;

	if (!slm_uart_can_context_send(data, len)) {
		return -EINTR;
	}
	k_mutex_lock(&mutex_tx_put, K_FOREVER);
	while (sent < len) {
		ret = ring_buf_put(&tx_buf, data + sent, len - sent);
		if (ret) {
			sent += ret;
		} else {
			/* Buffer full, block and start TX. */
			k_sem_take(&tx_done_sem, K_FOREVER);
			err = tx_start();
			if (err) {
				LOG_ERR("TX buf overflow, %d dropped. Unable to send: %d",
					len - sent,
					err);
				k_sem_give(&tx_done_sem);
				k_mutex_unlock(&mutex_tx_put);
				return err;
			}
		}
	}
	k_mutex_unlock(&mutex_tx_put);

	if (k_sem_take(&tx_done_sem, K_NO_WAIT) == 0) {
		err = tx_start();
		if (err == -ENODEV) {
			k_sem_give(&tx_done_sem);
			return 0;
		} else if (err) {
			LOG_ERR("TX start failed: %d", err);
			k_sem_give(&tx_done_sem);
			return err;
		}
	} else {
		/* TX already in progress. */
	}

	LOG_HEXDUMP_DBG(data, print_full_debug ? len : MIN(HEXDUMP_LIMIT, len), "TX");

	return 0;
}

int slm_uart_handler_init(slm_uart_rx_callback_t callback_t)
{
	int err;
	uint32_t start_time;

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}
	if (!slm_uart_configured) {
		/* Save UART configuration to settings page */
		slm_uart_configured = true;
		err = uart_config_get(uart_dev, &slm_uart);
		if (err != 0) {
			LOG_ERR("uart_config_get: %d", err);
			return err;
		}
		err = slm_settings_uart_save();
		if (err != 0) {
			LOG_ERR("slm_settings_uart_save: %d", err);
			return err;
		}
	} else {
		/* Configure UART based on settings page */
		err = slm_uart_configure();
		if (err != 0) {
			LOG_ERR("Fail to set UART baudrate: %d", err);
			return err;
		}
	}
	LOG_INF("UART baud: %d d/p/s-bits: %d/%d/%d HWFC: %d",
		slm_uart.baudrate, slm_uart.data_bits, slm_uart.parity,
		slm_uart.stop_bits, slm_uart.flow_ctrl);

	/* Wait for the UART line to become valid */
	start_time = k_uptime_get_32();
	do {
		err = uart_err_check(uart_dev);
		if (err) {
			uint32_t now = k_uptime_get_32();

			if (now - start_time > UART_ERROR_DELAY_MS) {
				LOG_ERR("UART check failed: %d", err);
				return -EIO;
			}
			k_sleep(K_MSEC(10));
		}
	} while (err);
	err = uart_callback_set(uart_dev, uart_callback, NULL);
	if (err) {
		LOG_ERR("Cannot set callback: %d", err);
		return -EFAULT;
	}
	rx_callback_t = callback_t;
	(void)atomic_set(&recovery_state, RECOVERY_IDLE);
	err = rx_enable();
	if (err) {
		return -EFAULT;
	}

	k_work_init_delayable(&rx_process_work, rx_process);

	k_sem_give(&tx_done_sem);

	err = slm_uart_tx_write(SLM_SYNC_STR, sizeof(SLM_SYNC_STR)-1, true);
	if (err) {
		return err;
	}

	return 0;
}

void slm_uart_handler_uninit(void)
{
	int err;

	/* Power off UART module */
	rx_disable();
	err = pm_device_action_run(uart_dev, PM_DEVICE_ACTION_SUSPEND);
	if (err) {
		LOG_WRN("Can't suspend UART: %d", err);
	}
}
