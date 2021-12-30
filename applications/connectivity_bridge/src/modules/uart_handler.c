/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <sys/ring_buffer.h>
#include <drivers/uart.h>
#include <pm/device.h>

#define MODULE uart_handler
#include "module_state_event.h"
#include "peer_conn_event.h"
#include "ble_data_event.h"
#include "cdc_data_event.h"
#include "uart_data_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_BRIDGE_UART_LOG_LEVEL);

#define UART_BUF_SIZE CONFIG_BRIDGE_BUF_SIZE

#define UART_SLAB_BLOCK_SIZE sizeof(struct uart_rx_buf)
#define UART_SLAB_BLOCK_COUNT (UART_DEVICE_COUNT * CONFIG_BRIDGE_UART_BUF_COUNT)
#define UART_SLAB_ALIGNMENT 4
#define UART_RX_TIMEOUT_MS 1

#if defined(CONFIG_PM_DEVICE)
#define UART_SET_PM_STATE true
#else
#define UART_SET_PM_STATE false
#endif

#define UART_DEVICE_LIST \
	X(0)\
	X(1)

enum uart_device_idx {
#define X(_DEV_IDX) CONCAT(UART_DEVICE, _DEV_IDX),
	UART_DEVICE_LIST
#undef X
	UART_DEVICE_COUNT
};

BUILD_ASSERT(UART_DEVICE_COUNT > 0);

/* List of UART device names. "UART_0", "UART_1", etc. */
static const char *device_names[UART_DEVICE_COUNT] = {
#define X(_DEV_IDX) STRINGIFY(CONCAT(UART_, _DEV_IDX)),
	UART_DEVICE_LIST
#undef X
};

struct uart_rx_buf {
	atomic_t ref_counter;
	size_t len;
	uint8_t buf[UART_BUF_SIZE];
};

struct uart_tx_buf {
	struct ring_buf rb;
	uint32_t buf[UART_BUF_SIZE];
};

BUILD_ASSERT((sizeof(struct uart_rx_buf) % UART_SLAB_ALIGNMENT) == 0);

/* Blocks from the same slab is used for RX for all UART instances */
/* TX has inidividual ringbuffers per UART instance */

K_MEM_SLAB_DEFINE(uart_rx_slab, UART_SLAB_BLOCK_SIZE, UART_SLAB_BLOCK_COUNT, UART_SLAB_ALIGNMENT);

static const struct device *devices[UART_DEVICE_COUNT];
static struct uart_tx_buf uart_tx_ringbufs[UART_DEVICE_COUNT];
static uint32_t uart_default_baudrate[UART_DEVICE_COUNT];
/* UART RX only enabled when there is one or more subscribers (power saving) */
static int subscriber_count[UART_DEVICE_COUNT];
static bool enable_rx_retry[UART_DEVICE_COUNT];
static atomic_t uart_tx_started[UART_DEVICE_COUNT];

static void enable_uart_rx(uint8_t dev_idx);
static void disable_uart_rx(uint8_t dev_idx);
static void set_uart_power_state(uint8_t dev_idx, bool active);
static int uart_tx_start(uint8_t dev_idx);
static void uart_tx_finish(uint8_t dev_idx, size_t len);

static inline struct uart_rx_buf *block_start_get(uint8_t *buf)
{
	size_t block_num;

	/* blocks are fixed size units from a continuous memory slab: */
	/* round down to the closest unit size to find beginning of block. */

	block_num =
		(((size_t)buf - (size_t)uart_rx_slab.buffer) / UART_SLAB_BLOCK_SIZE);

	return (struct uart_rx_buf *) &uart_rx_slab.buffer[block_num * UART_SLAB_BLOCK_SIZE];
}

static struct uart_rx_buf *uart_rx_buf_alloc(void)
{
	struct uart_rx_buf *buf;
	int err;

	/* Async UART driver returns pointers to received data as */
	/* offsets from beginning of RX buffer block. */
	/* This code uses a reference counter to keep track of the number of */
	/* references within a single RX buffer block */

	err = k_mem_slab_alloc(&uart_rx_slab, (void **) &buf, K_NO_WAIT);
	if (err) {
		return NULL;
	}

	atomic_set(&buf->ref_counter, 1);

	return buf;
}

static void uart_rx_buf_ref(void *buf)
{
	__ASSERT_NO_MSG(buf);

	atomic_inc(&(block_start_get(buf)->ref_counter));
}

static void uart_rx_buf_unref(void *buf)
{
	__ASSERT_NO_MSG(buf);

	struct uart_rx_buf *uart_buf = block_start_get(buf);
	atomic_t ref_counter = atomic_dec(&uart_buf->ref_counter);

	/* ref_counter is the uart_buf->ref_counter value prior to decrement */
	if (ref_counter == 1) {
		k_mem_slab_free(&uart_rx_slab, (void **)&uart_buf);
	}
}

static void uart_callback(const struct device *dev, struct uart_event *evt,
			  void *user_data)
{
	int dev_idx = (int) user_data;
	struct uart_data_event *event;
	struct uart_rx_buf *buf;
	int err;

	switch (evt->type) {
	case UART_RX_RDY:
		uart_rx_buf_ref(evt->data.rx.buf);

		event = new_uart_data_event();
		event->dev_idx = dev_idx;
		event->buf = &evt->data.rx.buf[evt->data.rx.offset];
		event->len = evt->data.rx.len;
		EVENT_SUBMIT(event);
		break;
	case UART_RX_BUF_RELEASED:
		if (evt->data.rx_buf.buf) {
			uart_rx_buf_unref(evt->data.rx_buf.buf);
		}
		break;
	case UART_RX_BUF_REQUEST:
		buf = uart_rx_buf_alloc();
		if (buf == NULL) {
			LOG_WRN("UART_%d RX overflow", dev_idx);
			break;
		}

		err = uart_rx_buf_rsp(dev, buf->buf, sizeof(buf->buf));
		if (err) {
			LOG_ERR("uart_rx_buf_rsp: %d", err);
			uart_rx_buf_unref(buf);
		}
		break;
	case UART_RX_DISABLED:
		if (enable_rx_retry[dev_idx]) {
			enable_uart_rx(dev_idx);
			enable_rx_retry[dev_idx] = false;
		} else if (UART_SET_PM_STATE) {
			set_uart_power_state(dev_idx, false);
		}
		break;
	case UART_TX_DONE:
		uart_tx_finish(dev_idx, evt->data.tx.len);

		if (ring_buf_is_empty(&uart_tx_ringbufs[dev_idx].rb)) {
			atomic_set(&uart_tx_started[dev_idx], false);
		} else {
			uart_tx_start(dev_idx);
		}
		break;
	case UART_TX_ABORTED:
		uart_tx_finish(dev_idx, evt->data.tx.len);
		atomic_set(&uart_tx_started[dev_idx], false);
		break;
	case UART_RX_STOPPED:
		LOG_WRN("UART_%d stop reason %d", dev_idx, evt->data.rx_stop.reason);

		/* Retry automatically in case of unexpected stop.
		 * Typically happens when the peer does not drive its TX GPIO,
		 * or if there is a baud rate mismatch.
		 */
		enable_rx_retry[dev_idx] = true;
		break;
	default:
		LOG_ERR("Unexpected event: %d", evt->type);
		__ASSERT_NO_MSG(false);
		break;
	}
}

static void set_uart_baudrate(uint8_t dev_idx, uint32_t baudrate)
{
	const struct device *dev = devices[dev_idx];
	struct uart_config cfg;
	int err;

	if (baudrate == 0) {
		return;
	}

	err = uart_config_get(dev, &cfg);
	if (err) {
		LOG_ERR("uart_config_get: %d", err);
		return;
	}

	if (cfg.baudrate == baudrate) {
		return;
	}

	cfg.baudrate = baudrate;

	err = uart_configure(dev, &cfg);
	if (err) {
		LOG_ERR("uart_configure: %d", err);
		return;
	}
}

static void set_uart_power_state(uint8_t dev_idx, bool active)
{
#if UART_SET_PM_STATE
	const struct device *dev = devices[dev_idx];
	int err;
	enum pm_device_action action;

	action = active ? PM_DEVICE_ACTION_RESUME : PM_DEVICE_ACTION_SUSPEND;

	err = pm_device_action_run(dev, action);
	if ((err < 0) && (err != -EALREADY)) {
		LOG_ERR("pm_device_action_run failed: %d", err);
	}
#endif
}

static void enable_uart_rx(uint8_t dev_idx)
{
	const struct device *dev = devices[dev_idx];
	int err;
	struct uart_rx_buf *buf;

	err = uart_callback_set(dev, uart_callback, (void *) (int) dev_idx);
	if (err) {
		LOG_ERR("uart_callback_set: %d", err);
		return;
	}

	buf = uart_rx_buf_alloc();
	if (!buf) {
		LOG_ERR("uart_rx_buf_alloc error");
		return;
	}

	err = uart_rx_enable(dev, buf->buf, sizeof(buf->buf), UART_RX_TIMEOUT_MS);
	if (err) {
		uart_rx_buf_unref(buf);
		LOG_ERR("uart_rx_enable: %d", err);
		return;
	}
}

static void disable_uart_rx(uint8_t dev_idx)
{
	const struct device *dev = devices[dev_idx];
	int err;

	err = uart_rx_disable(dev);
	if (err) {
		LOG_ERR("uart_rx_disable: %d", err);
		return;
	}
}

static int uart_tx_start(uint8_t dev_idx)
{
	int len;
	int err;
	uint8_t *buf;

	len = ring_buf_get_claim(
			&uart_tx_ringbufs[dev_idx].rb,
			&buf,
			sizeof(uart_tx_ringbufs[dev_idx].buf));

	err = uart_tx(devices[dev_idx], buf, len, 0);
	if (err) {
		LOG_ERR("uart_tx: %d", err);
		uart_tx_finish(dev_idx, 0);
		return err;
	}

	return 0;
}

static void uart_tx_finish(uint8_t dev_idx, size_t len)
{
	int err;

	err = ring_buf_get_finish(&uart_tx_ringbufs[dev_idx].rb, len);
	if (err) {
		LOG_ERR("ring_buf_get_finish: %d", err);
	}
}

static int uart_tx_enqueue(uint8_t *data, size_t data_len, uint8_t dev_idx)
{
	atomic_t started;
	uint32_t written;
	int err;

	written = ring_buf_put(&uart_tx_ringbufs[dev_idx].rb, data, data_len);
	if (written == 0) {
		return -ENOMEM;
	}

	started = atomic_set(&uart_tx_started[dev_idx], true);
	if (!started) {
		err = uart_tx_start(dev_idx);
		if (err) {
			LOG_ERR("uart_tx_start: %d", err);
			atomic_set(&uart_tx_started[dev_idx], false);
		}
	}

	if (written == data_len) {
		return 0;
	} else {
		return -ENOMEM;
	}

	return 0;
}

static bool event_handler(const struct event_header *eh)
{
	int err;

	if (is_uart_data_event(eh)) {
		const struct uart_data_event *event =
			cast_uart_data_event(eh);

		/* All subscribers have gotten a chance to copy data at this point */
		uart_rx_buf_unref(event->buf);

		return true;
	}

	if (is_cdc_data_event(eh)) {
		const struct cdc_data_event *event =
			cast_cdc_data_event(eh);

		if (event->dev_idx >= UART_DEVICE_COUNT) {
			return false;
		}

		if (!devices[event->dev_idx]) {
			return false;
		}

		err = uart_tx_enqueue(event->buf, event->len, event->dev_idx);
		if (err == -ENOMEM) {
			LOG_WRN("CDC_%d->UART_%d overflow",
				event->dev_idx,
				event->dev_idx);
		} else if (err) {
			LOG_ERR("uart_tx_enqueue: %d", err);
		}

		return false;
	}

	if (is_ble_data_event(eh)) {
		const struct ble_data_event *event =
			cast_ble_data_event(eh);
		/* Only one BLE Service instance: always map to UART_0 */
		uint8_t dev_idx = 0;

		if (!devices[dev_idx]) {
			return false;
		}

		err = uart_tx_enqueue(event->buf, event->len, dev_idx);
		if (err == -ENOMEM) {
			LOG_WRN("BLE->UART_%d overflow", dev_idx);
		} else if (err) {
			LOG_ERR("uart_tx_enqueue: %d", err);
		}

		return false;
	}

	if (is_peer_conn_event(eh)) {
		const struct peer_conn_event *event =
			cast_peer_conn_event(eh);
		int prev_count;

		if (event->dev_idx >= UART_DEVICE_COUNT) {
			return false;
		}

		if (!devices[event->dev_idx]) {
			return false;
		}

		prev_count = subscriber_count[event->dev_idx];

		if (event->conn_state == PEER_STATE_CONNECTED) {
			subscriber_count[event->dev_idx] += 1;
			set_uart_baudrate(event->dev_idx, event->baudrate);
		} else {
			subscriber_count[event->dev_idx] -= 1;
		}

		__ASSERT_NO_MSG(subscriber_count[event->dev_idx] >= 0);

		if (subscriber_count[event->dev_idx] == 0) {
			LOG_DBG("No subscribers. Close UART_%d RX", event->dev_idx);
			set_uart_baudrate(
				event->dev_idx,
				uart_default_baudrate[event->dev_idx]);
			disable_uart_rx(event->dev_idx);
		} else if (prev_count == 0) {
			LOG_DBG("First subscriber. Open UART_%d RX", event->dev_idx);
			if (UART_SET_PM_STATE) {
				set_uart_power_state(event->dev_idx, true);
			}
			enable_uart_rx(event->dev_idx);
		} else {
			return false;
		}

		return false;
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			for (int i = 0; i < UART_DEVICE_COUNT; ++i) {
				struct uart_config cfg;

				devices[i] = device_get_binding(device_names[i]);
				if (!devices[i]) {
					LOG_ERR("%s not available", log_strdup(device_names[i]));
					continue;
				}

				err = uart_config_get(devices[i], &cfg);
				if (err) {
					LOG_ERR("uart_config_get: %d", err);
					return false;
				}
				uart_default_baudrate[i] = cfg.baudrate;
				subscriber_count[i] = 0;
				enable_rx_retry[i] = false;

				atomic_set(&uart_tx_started[i], false);

				ring_buf_init(
					&uart_tx_ringbufs[i].rb,
					sizeof(uart_tx_ringbufs[i].buf),
					uart_tx_ringbufs[i].buf);

				if (UART_SET_PM_STATE) {
					set_uart_power_state(i, false);
				}
			}
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, peer_conn_event);
EVENT_SUBSCRIBE(MODULE, ble_data_event);
EVENT_SUBSCRIBE(MODULE, cdc_data_event);
EVENT_SUBSCRIBE_FINAL(MODULE, uart_data_event);
