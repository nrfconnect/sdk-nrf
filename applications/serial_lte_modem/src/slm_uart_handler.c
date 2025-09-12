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
#include <assert.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/pm/device.h>
#include <zephyr/modem/pipe.h>
#include "slm_uart_handler.h"
#include "slm_at_host.h"
#include <hal/nrf_regulators.h>
#include <zephyr/sys/reboot.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(slm_uart_handler, CONFIG_SLM_LOG_LEVEL);

#define SLM_PIPE CONFIG_SLM_CMUX

#define UART_RX_TIMEOUT_US		2000
#define UART_ERROR_DELAY_MS		500

const struct device *const slm_uart_dev = DEVICE_DT_GET(DT_CHOSEN(ncs_slm_uart));
uint32_t slm_uart_baudrate;

static struct k_work_delayable rx_process_work;
static struct k_work sleep_work;

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

enum rx_event_owner {
	RX_EVENT_OWNER_AT = 0,
	RX_EVENT_OWNER_PIPE
};
struct pending_rx_event_t {
	enum rx_event_owner owner;
	struct rx_event_t event;
};
static struct pending_rx_event_t pending_rx_event;

RING_BUF_DECLARE(tx_buf, CONFIG_SLM_UART_TX_BUF_SIZE);
K_MUTEX_DEFINE(mutex_tx_put); /* Protects the tx_buf from multiple writes. */

enum slm_uart_state {
	SLM_TX_ENABLED_BIT,
	SLM_RX_ENABLED_BIT,
	SLM_RX_RECOVERY_BIT,
	SLM_RX_RECOVERY_DISABLED_BIT
};
static atomic_t uart_state;

#if SLM_PIPE

enum slm_pipe_state {
	SLM_PIPE_INIT_BIT,
	SLM_PIPE_OPEN_BIT,
};
static struct {
	struct modem_pipe pipe;
	slm_pipe_tx_t tx_cb;
	atomic_t state;

	struct k_work notify_transmit_idle;
	struct k_work notify_closed;
} slm_pipe;

#endif

K_SEM_DEFINE(tx_done_sem, 0, 1);

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
		k_mem_slab_free(&rx_slab, (void *)uart_buf);
	}
}

static int rx_enable(void)
{
	struct rx_buf_t *buf;
	int ret;

	if (atomic_test_bit(&uart_state, SLM_RX_ENABLED_BIT)) {
		return 0;
	}

	buf = rx_buf_alloc();
	if (!buf) {
		LOG_ERR("UART RX failed to allocate buffer");
		return -ENOMEM;
	}

	ret = uart_rx_enable(slm_uart_dev, buf->buf, sizeof(buf->buf), UART_RX_TIMEOUT_US);
	if (ret) {
		LOG_ERR("UART RX enable failed: %d", ret);
		rx_buf_unref(buf->buf);
		return ret;
	}
	atomic_set_bit(&uart_state, SLM_RX_ENABLED_BIT);

	return 0;
}

static int rx_disable(void)
{
	int err;

	atomic_set_bit(&uart_state, SLM_RX_RECOVERY_DISABLED_BIT);

	while (atomic_test_bit(&uart_state, SLM_RX_RECOVERY_BIT)) {
		/* Wait until possible recovery is complete. */
		k_sleep(K_MSEC(10));
	}

	err = uart_rx_disable(slm_uart_dev);
	if (err) {
		LOG_ERR("UART RX disable failed: %d", err);
		return err;
	}

	return 0;
}

static void rx_recovery(void)
{
	int err;

	if (atomic_test_bit(&uart_state, SLM_RX_RECOVERY_DISABLED_BIT)) {
		return;
	}

	atomic_set_bit(&uart_state, SLM_RX_RECOVERY_BIT);

	err = rx_enable();
	if (err) {
		k_work_schedule(&rx_process_work, K_MSEC(UART_RX_MARGIN_MS));
	}

	atomic_clear_bit(&uart_state, SLM_RX_RECOVERY_BIT);
}

static void rx_process(struct k_work *work)
{
#if SLM_PIPE
	/* With pipe, CMUX layer is notified and it requests data. */
	if (atomic_test_bit(&slm_pipe.state, SLM_PIPE_INIT_BIT)) {
		if (atomic_test_bit(&slm_pipe.state, SLM_PIPE_OPEN_BIT)) {
			modem_pipe_notify_receive_ready(&slm_pipe.pipe);
		}
		return;
	}
#endif
	/* Without pipe, we push the data immediately. */
	size_t processed;
	bool stop_at_receive = false;

	__ASSERT(pending_rx_event.owner == RX_EVENT_OWNER_AT && pending_rx_event.event.len == 0,
		 "Invalid pending RX event.");

	while (k_msgq_get(&rx_event_queue, &pending_rx_event.event, K_NO_WAIT) == 0) {
		processed = slm_at_receive(pending_rx_event.event.buf, pending_rx_event.event.len,
					   &stop_at_receive);

		if (processed == pending_rx_event.event.len) {
			/* All data processed, release the buffer. */
			rx_buf_unref(pending_rx_event.event.buf);
			pending_rx_event.event.buf = NULL;
			pending_rx_event.event.len = 0;
		} else {
			/* Partial data processing can only be due to CMUX change. */
			pending_rx_event.event.buf += processed;
			pending_rx_event.event.len -= processed;
		}

		if (stop_at_receive) {
			pending_rx_event.owner = RX_EVENT_OWNER_PIPE;
			break;
		}
	}

	rx_recovery();
}

static void tx_enable(void)
{
	if (!atomic_test_and_set_bit(&uart_state, SLM_TX_ENABLED_BIT)) {
		k_sem_give(&tx_done_sem);
	}
}

static int tx_disable(k_timeout_t timeout)
{
	int err;

	if (!atomic_test_and_clear_bit(&uart_state, SLM_TX_ENABLED_BIT)) {
		return 0;
	}

	if (k_sem_take(&tx_done_sem, timeout) == 0) {
		return 0;
	}

	err = uart_tx_abort(slm_uart_dev);
	if (!err) {
		LOG_INF("TX aborted");
	} else if (err != -EFAULT) {
		LOG_ERR("uart_tx_abort failed (%d).", err);
		return err;
	}

	return 0;
}

static int tx_start(void)
{
	uint8_t *buf;
	size_t len;
	int err;
	// enum pm_device_state state = PM_DEVICE_STATE_OFF;

	// pm_device_state_get(slm_uart_dev, &state);
	// if (state != PM_DEVICE_STATE_ACTIVE) {
	// 	return 1;
	// }
	if (!atomic_test_bit(&uart_state, SLM_TX_ENABLED_BIT)) {
		return -EAGAIN;
	}

	len = ring_buf_get_claim(&tx_buf, &buf, ring_buf_capacity_get(&tx_buf));
	err = uart_tx(slm_uart_dev, buf, len, SYS_FOREVER_US);
	if (err) {
		LOG_ERR("UART TX error: %d", err);
		ring_buf_get_finish(&tx_buf, 0);
		return err;
	}

	return 0;
}

static inline void uart_callback_notify_pipe_transmit_idle(void)
{
#if SLM_PIPE
	if (atomic_test_bit(&slm_pipe.state, SLM_PIPE_OPEN_BIT)) {
		k_work_submit(&slm_pipe.notify_transmit_idle);
	}
#endif
}

static inline void uart_callback_notify_pipe_closure(void)
{
#if SLM_PIPE
	if (atomic_test_bit(&slm_pipe.state, SLM_PIPE_INIT_BIT) &&
	    !atomic_test_bit(&slm_pipe.state, SLM_PIPE_OPEN_BIT) &&
	    !atomic_test_bit(&uart_state, SLM_RX_ENABLED_BIT) &&
	    !atomic_test_bit(&uart_state, SLM_TX_ENABLED_BIT)) {
		/* Pipe is closed, RX and TX are idle, notify the closure */
		k_work_submit(&slm_pipe.notify_closed);
	}
#endif
}

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	struct rx_buf_t *buf;
	struct rx_event_t rx_event;
	int err;
	static bool hwfc_active;

	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case UART_TX_DONE:
	case UART_TX_ABORTED:
		err = ring_buf_get_finish(&tx_buf, evt->data.tx.len);
		if (err) {
			LOG_ERR("UART_TX_%s failure: %d",
				(evt->type == UART_TX_DONE) ? "DONE" : "ABORTED", err);
		}
		if (ring_buf_is_empty(&tx_buf) == false && evt->type == UART_TX_DONE) {
			tx_start();
		} else {
			k_sem_give(&tx_done_sem);
			uart_callback_notify_pipe_transmit_idle();
		}
		break;
	case UART_RX_RDY:
		rx_buf_ref(evt->data.rx.buf);
		rx_event.buf = &evt->data.rx.buf[evt->data.rx.offset];
		rx_event.len = evt->data.rx.len;
		err = k_msgq_put(&rx_event_queue, &rx_event, K_NO_WAIT);
		if (err) {
			LOG_ERR("RX event queue full, dropped %zu bytes", evt->data.rx.len);
			rx_buf_unref(evt->data.rx.buf);
			break;
		}
		(void)k_work_submit((struct k_work *)&rx_process_work);
		break;
	case UART_RX_BUF_REQUEST:
		hwfc_active = false;
		if (k_msgq_num_free_get(&rx_event_queue) < UART_RX_EVENT_COUNT_FOR_BUF) {
			LOG_WRN("Disabling UART RX: No event space.");
			break;
		}
		buf = rx_buf_alloc();
		if (!buf) {
			hwfc_active = true;
			LOG_WRN("Disabling UART RX: No free buffers.");
			break;
		}
		err = uart_rx_buf_rsp(slm_uart_dev, buf->buf, sizeof(buf->buf));
		if (err) {
			hwfc_active = true;
			LOG_WRN("Disabling UART RX: %d", err);
			rx_buf_unref(buf);
		}
		break;
	case UART_RX_BUF_RELEASED:
		if (evt->data.rx_buf.buf) {
			rx_buf_unref(evt->data.rx_buf.buf);
		}
		break;
	case UART_RX_DISABLED:
		LOG_INF("UART RX disabled");
		atomic_clear_bit(&uart_state, SLM_RX_ENABLED_BIT);
		if (hwfc_active) {
			LOG_INF("UART RX disabled due to flow control");
			k_work_submit((struct k_work *)&rx_process_work);
		}
		// MARKUS TODO: Whether we shutdown or go to sleep should
		// be configurable with AT-commands.
		// (void)k_work_submit((struct k_work *)&sleep_work);
		break;
	default:
		break;
	}

	uart_callback_notify_pipe_closure();
}

/* Write the data to tx_buffer and trigger sending. Repeat until everything is sent.
 * Returns 0 on success or a negative error code.
 */
static int slm_uart_tx_write(const uint8_t *data, size_t len)
{
	size_t ret;
	size_t sent = 0;
	int err;

	k_mutex_lock(&mutex_tx_put, K_FOREVER);

	while (sent < len) {
		ret = ring_buf_put(&tx_buf, data + sent, len - sent);
		if (ret) {
			sent += ret;
			continue;
		}

		/* Buffer full, block and start TX. */
		err = k_sem_take(&tx_done_sem, K_FOREVER);
		if (err) {
			LOG_ERR("TX %s failed (%d). TX buf overflow, %zu dropped.",
				"semaphore take", err, len - sent);
			k_mutex_unlock(&mutex_tx_put);
			return err;
		}
		err = tx_start();
		if (err && err != -EAGAIN) {
			LOG_ERR("TX %s failed (%d). TX buf overflow, %zu dropped.", "start", err,
				len - sent);
			k_sem_give(&tx_done_sem);
			k_mutex_unlock(&mutex_tx_put);
			return err;
		}
	}
	k_mutex_unlock(&mutex_tx_put);

	if (k_sem_take(&tx_done_sem, K_NO_WAIT) == 0) {
		err = tx_start();
		if (err == 1) {
			k_sem_give(&tx_done_sem);
			return 0;
		} else if (err && err != -EAGAIN) {
			LOG_ERR("TX %s failed (%d).", "start", err);
			k_sem_give(&tx_done_sem);
			return err;
		}
	} else {
		/* TX already in progress. */
	}

	return 0;
}

static void sleep_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	// MARKUS TODO: Some of the cleanup code sends UART responses. Those should be handled properly.
	// This is a problem with having the virtual uart driver. The API between that and the SLM is
	// the default UART API, which does not allow for us to react to DTR before closing the UART.
	//
	// It is possible to remove the UART responses in cleanup code. But it would be better to have them.

	slm_at_host_uninit();

	/* Only power off the modem if it has not been put
	 * in flight mode to allow reducing NVM wear.
	 */
	// if (!slm_is_modem_functional_mode(LTE_LC_FUNC_MODE_OFFLINE)) {
	// 	slm_power_off_modem();
	// }

	LOG_INF("Entering sleep.");
	// LOG_PANIC();
	// nrf_gpio_cfg_sense_set(CONFIG_SLM_POWER_PIN, NRF_GPIO_PIN_SENSE_LOW);

	k_sleep(K_MSEC(100));

	nrf_regulators_system_off(NRF_REGULATORS_NS);
	assert(false);
}

int slm_tx_write(const uint8_t *data, size_t len)
{
#if SLM_PIPE
	if (atomic_test_bit(&slm_pipe.state, SLM_PIPE_INIT_BIT) && slm_pipe.tx_cb != NULL) {
		return slm_pipe.tx_cb(data, len);
	}
#endif
	return slm_uart_tx_write(data, len);
}

int slm_uart_handler_enable(void)
{
	int err;
	uint32_t start_time;
	struct uart_config cfg;

	if (!device_is_ready(slm_uart_dev)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	err = uart_config_get(slm_uart_dev, &cfg);
	if (err) {
		LOG_ERR("uart_config_get: %d", err);
		return err;
	}

	slm_uart_baudrate = cfg.baudrate;
	LOG_INF("UART baud: %d d/p/s-bits: %d/%d/%d HWFC: %d",
		cfg.baudrate, cfg.data_bits, cfg.parity,
		cfg.stop_bits, cfg.flow_ctrl);

	/* Wait for the UART line to become valid */
	start_time = k_uptime_get_32();
	do {
		err = uart_err_check(slm_uart_dev);
		if (err) {
			uint32_t now = k_uptime_get_32();

			if (now - start_time > UART_ERROR_DELAY_MS) {
				LOG_ERR("UART check failed: %d", err);
				return -EIO;
			}
			k_sleep(K_MSEC(10));
		}
	} while (err);
	err = uart_callback_set(slm_uart_dev, uart_callback, NULL);
	if (err) {
		LOG_ERR("Cannot set callback: %d", err);
		return -EFAULT;
	}

	atomic_clear(&uart_state);
	tx_enable();
	err = rx_enable();
	if (err) {
		return -EFAULT;
	}

	k_work_init_delayable(&rx_process_work, rx_process);
	k_work_init(&sleep_work, sleep_work_fn);


	/* Flush possibly pending data in case SLM was idle. */
	tx_start();

	return 0;
}

int slm_uart_handler_disable(void)
{
	int err;

	err = tx_disable(K_MSEC(50));
	if (err) {
		LOG_ERR("TX disable failed (%d).", err);
		return err;
	}

	err = rx_disable();
	if (err) {
		LOG_ERR("RX disable failed (%d).", err);
		return err;
	}

	return 0;
}

#if SLM_PIPE

static int pipe_open(void *data)
{
	int ret;

	ARG_UNUSED(data);

	if (!atomic_test_bit(&slm_pipe.state, SLM_PIPE_INIT_BIT)) {
		return -EINVAL;
	}

	if (atomic_test_bit(&slm_pipe.state, SLM_PIPE_OPEN_BIT)) {
		return -EALREADY;
	}

	atomic_clear_bit(&uart_state, SLM_RX_RECOVERY_DISABLED_BIT);
	ret = rx_enable();
	if (ret) {
		return ret;
	}

	tx_enable();

	atomic_set_bit(&slm_pipe.state, SLM_PIPE_OPEN_BIT);
	modem_pipe_notify_opened(&slm_pipe.pipe);

	return 0;
}

/* Returns the number of bytes written or a negative error code. */
static int pipe_transmit(void *data, const uint8_t *buf, size_t size)
{
	size_t ret;
	size_t sent = 0;

	ARG_UNUSED(data);

	if (!atomic_test_bit(&slm_pipe.state, SLM_PIPE_OPEN_BIT)) {
		return -EPERM;
	}

	if (!buf || size == 0) {
		return -EINVAL;
	}

	while (sent < size) {
		ret = ring_buf_put(&tx_buf, buf + sent, size - sent);
		if (ret) {
			sent += ret;
		} else {
			/* Buffer full. */
			break;
		}
	}

	if (k_sem_take(&tx_done_sem, K_NO_WAIT) == 0) {
		int err = tx_start();

		if (err == 1 || err == -EAGAIN) {
			k_sem_give(&tx_done_sem);
			return (int)sent;
		} else if (err) {
			LOG_ERR("TX %s failed (%d).", "start", err);
			k_sem_give(&tx_done_sem);
			return err;
		}
	}

	return (int)sent;
}

static int pipe_receive(void *data, uint8_t *buf, size_t size)
{
	size_t received = 0;
	size_t copy_size;

	ARG_UNUSED(data);

	if (!buf || size == 0) {
		return 0;
	}

	__ASSERT(pending_rx_event.owner == RX_EVENT_OWNER_PIPE,
		 "RX event buffer not owned by pipe");

	while (size > received) {
		/* Fetch a new event only when the current one is fully consumed */
		if (pending_rx_event.event.len == 0) {
			if (k_msgq_get(&rx_event_queue, &pending_rx_event.event, K_NO_WAIT)) {
				break;
			}
		}
		copy_size = MIN(size - received, pending_rx_event.event.len);
		memcpy(buf, pending_rx_event.event.buf, copy_size);
		received += copy_size;
		buf += copy_size;

		if (pending_rx_event.event.len == copy_size) {
			rx_buf_unref(pending_rx_event.event.buf);
			pending_rx_event.event.buf = NULL;
			pending_rx_event.event.len = 0;
		} else {
			pending_rx_event.event.buf += copy_size;
			pending_rx_event.event.len -= copy_size;
		}
	}
	if (pending_rx_event.event.len == 0) {
		/* All events consumed, try to recover RX if it was disabled. */
		rx_recovery();
	}

	return (int)received;
}

static int pipe_close(void *data)
{
	int err;

	ARG_UNUSED(data);

	if (!atomic_test_bit(&slm_pipe.state, SLM_PIPE_OPEN_BIT)) {
		return -EALREADY;
	}

	atomic_clear_bit(&slm_pipe.state, SLM_PIPE_OPEN_BIT);

	err = tx_disable(K_MSEC(50));
	if (err) {
		LOG_WRN("%s disable failed (%d).", "TX", err);
	}

	err = rx_disable();
	if (err) {
		LOG_ERR("%s disable failed (%d).", "RX", err);
	}

	return 0;
}

static const struct modem_pipe_api modem_pipe_api = {
	.open = pipe_open,
	.transmit = pipe_transmit,
	.receive = pipe_receive,
	.close = pipe_close,
};

static void notify_transmit_idle_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	modem_pipe_notify_transmit_idle(&slm_pipe.pipe);
}
static void notify_closed_fn(struct k_work *work)
{
	ARG_UNUSED(work);
	modem_pipe_notify_closed(&slm_pipe.pipe);
}

static void at_to_cmux_switch(void)
{
	/* TX handling when moving from AT to CMUX:
	 * - Complete (OK message) TX transmission through regular UART.
	 */
	tx_disable(K_MSEC(10));

	/* Possible TX handling improvements:
	 * - Callers in slm_uart_tx_write need to abort when CMUX change is done. There may be
	 *   multiple callers and they may be blocked.
	 * - TX buffer must be flushed or the data must be routed to CMUX.
	 */

	/* RX handling when moving from AT to CMUX:
	 * - RX and RX buffers are retained.
	 * - Data in RX buffers is routed to CMUX AT channel.
	 */
}

struct modem_pipe *slm_uart_pipe_init(slm_pipe_tx_t pipe_tx_cb)
{
	k_work_init(&slm_pipe.notify_transmit_idle, notify_transmit_idle_fn);
	k_work_init(&slm_pipe.notify_closed, notify_closed_fn);

	slm_pipe.tx_cb = pipe_tx_cb;
	atomic_set_bit(&slm_pipe.state, SLM_PIPE_INIT_BIT);

	modem_pipe_init(&slm_pipe.pipe, &slm_pipe, &modem_pipe_api);

	at_to_cmux_switch();

	return &slm_pipe.pipe;
}

#endif /* SLM_PIPE */
