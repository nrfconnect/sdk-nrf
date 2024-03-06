/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief UART asynchronous API adapter implementation
 */
#include "uart_async_adapter.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_async_adapter);


#if !IS_ENABLED(CONFIG_UART_ASYNC_API)
#error "This adapter requires UART ASYNC API to be enabled"
#endif

#if !IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
#error "The adapter requires UART INTERRUPT API to be enabled"
#endif


/**
 * @brief Access the data inside device
 *
 * This function gets the pointer to the user data inside device
 *
 * @param dev Device to get adapter data
 * @return Pointer to device data
 */
static struct uart_async_adapter_data *access_dev_data(const struct device *dev)
{
	__ASSERT_NO_MSG(dev);
	__ASSERT_NO_MSG(dev->data);
	return (struct uart_async_adapter_data *)(dev->data);
}

/**
 * @brief Call the sync callback function
 *
 * @param dev UART device structure.
 * @param evt Pointer to uart_event structure.
 */
static void user_callback(const struct device *dev, struct uart_event *evt)
{
	struct uart_async_adapter_data *data = access_dev_data(dev);

	if (data->user_callback) {
		data->user_callback(dev, evt, data->user_data);
	}
}

static int callback_set(const struct device *dev, uart_callback_t callback, void *user_data)
{
	struct uart_async_adapter_data *data = access_dev_data(dev);

	data->user_callback = callback;
	data->user_data = user_data;

	return 0;
}

static inline void notify_rx_buffer(const struct device *dev)
{
	struct uart_event event = {UART_RX_RDY};
	struct uart_async_adapter_data *data = access_dev_data(dev);
	bool notify = false;

	k_spinlock_key_t key = k_spin_lock(&(data->lock));

	if (data->rx.buf) {

		__ASSERT_NO_MSG(data->rx.curr_buf >= data->rx.buf);
		__ASSERT_NO_MSG(data->rx.curr_buf >= data->rx.last_notify_buf);
		if (data->rx.curr_buf > data->rx.last_notify_buf) {

			event.data.rx.buf = data->rx.buf;
			event.data.rx.len = data->rx.curr_buf - data->rx.last_notify_buf;
			event.data.rx.offset = data->rx.last_notify_buf - data->rx.buf;
			data->rx.last_notify_buf = data->rx.curr_buf;
			notify = true;
		}
	}

	k_spin_unlock(&(data->lock), key);

	if (notify) {
		LOG_DBG("Notification: UART_RX_RDY (0x%x + %d, size: %d)",
			(unsigned int)event.data.rx.buf, event.data.rx.offset, event.data.rx.len);
		user_callback(dev, &event);
	}
}

static inline void notify_rx_buffer_release(const struct device *dev, uint8_t *buf)
{
	struct uart_event event = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = buf
	};
	LOG_DBG("Notification: UART_RX_BUF_RELEASED");
	user_callback(dev, &event);
}

static inline void notify_rx_buffer_req(const struct device *dev)
{
	struct uart_event event = {
		.type = UART_RX_BUF_REQUEST,
	};
	LOG_DBG("Notification: UART_RX_BUF_REQUEST");
	user_callback(dev, &event);
}

static inline void switch_rx_buffer(const struct device *dev, bool request_new)
{
	struct uart_async_adapter_data *data = access_dev_data(dev);
	uint8_t *old_buf;

	k_spinlock_key_t key = k_spin_lock(&(data->lock));

	old_buf = data->rx.buf;
	data->rx.buf = data->rx.next_buf;
	data->rx.curr_buf = data->rx.next_buf;
	data->rx.last_notify_buf = data->rx.next_buf;
	data->rx.size_left = data->rx.next_buf_len;
	data->rx.next_buf = NULL;
	data->rx.next_buf_len = 0;

	k_spin_unlock(&(data->lock), key);

	if (old_buf) {
		notify_rx_buffer_release(dev, old_buf);
	}
	if (request_new) {
		notify_rx_buffer_req(dev);
	}
}

static int tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout)
{
	int ret = 0;
	struct uart_async_adapter_data *data = access_dev_data(dev);
	bool tx_send = false;

	LOG_DBG("%s(%s, 0x%x, %u, %d)", __func__, dev->name, (unsigned int)buf, len, timeout);

	k_spinlock_key_t key = k_spin_lock(&(data->lock));

	if (!len) {
		LOG_DBG("%s: no data", __func__);
	} else if (data->tx.buf) {
		ret = -EBUSY;
		LOG_DBG("%s: busy", __func__);
	} else {
		data->tx.buf = buf;
		data->tx.curr_buf = buf;
		data->tx.size_left = len;
		data->tx.enabled = true;
		tx_send = true;
		LOG_DBG("%s: sending", __func__);
	}

	k_spin_unlock(&(data->lock), key);

	if (tx_send) {
		uart_irq_tx_enable(data->target);
	}
	if (tx_send && timeout != SYS_FOREVER_US) {
		k_timer_start(&data->tx.timeout_timer, K_USEC(timeout), K_NO_WAIT);
	}
	return ret;
}

static int tx_abort(const struct device *dev)
{
	int ret = 0;
	struct uart_event event = {UART_TX_ABORTED};
	struct uart_async_adapter_data *data = access_dev_data(dev);

	data->tx.enabled = false;
	k_timer_stop(&data->tx.timeout_timer);
	uart_irq_tx_disable(data->target);

	k_spinlock_key_t key = k_spin_lock(&(data->lock));

	if (data->tx.buf == NULL) {
		ret = -EFAULT;
	} else {
		/* Set the event data */
		event.data.tx.buf = data->tx.buf;
		event.data.tx.len = data->tx.curr_buf - data->tx.buf;
		/* Clear any pending tx */
		data->tx.buf = NULL;
		data->tx.curr_buf = NULL;
		data->tx.size_left = 0;
	}

	k_spin_unlock(&(data->lock), key);

	if (!ret) {
		user_callback(dev, &event);
	}
	return ret;
}


static int rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	int ret = 0;
	struct uart_async_adapter_data *data = access_dev_data(dev);

	k_spinlock_key_t key = k_spin_lock(&(data->lock));

	if (data->rx.next_buf || data->rx.buf) {
		ret = -EBUSY;
	}
	data->rx.buf = NULL;
	data->rx.curr_buf = NULL;
	data->rx.size_left = 0;
	data->rx.next_buf = buf;
	data->rx.next_buf_len = len;
	data->rx.timeout = timeout;
	data->rx.enabled = true;

	k_spin_unlock(&(data->lock), key);

	uart_irq_rx_enable(data->target);
	uart_irq_err_enable(data->target);
	return ret;
}

static int rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	int ret = 0;
	struct uart_async_adapter_data *data = access_dev_data(dev);

	k_spinlock_key_t key = k_spin_lock(&(data->lock));

	if (data->rx.next_buf) {
		ret = -EBUSY;
	} else {
		__ASSERT_NO_MSG(!data->rx.next_buf_len);
		data->rx.next_buf = buf;
		data->rx.next_buf_len = len;
	}

	k_spin_unlock(&(data->lock), key);

	return ret;
}

static int rx_disable(const struct device *dev)
{
	int ret = 0;
	struct uart_async_adapter_data *data = access_dev_data(dev);
	struct uart_event event_disabled = {UART_RX_DISABLED};

	data->rx.enabled = false;
	uart_irq_rx_disable(data->target);
	uart_irq_err_disable(data->target);
	while (data->rx.buf) {
		switch_rx_buffer(dev, false);
	}

	user_callback(dev, &event_disabled);

	return ret;
}

/* Basic interface */

static int poll_in(const struct device *dev, unsigned char *p_char)
{
	__ASSERT_NO_MSG(dev);

	const struct uart_async_adapter_data *data = dev->data;

	__ASSERT_NO_MSG(data);
	__ASSERT_NO_MSG(data->target);

	return uart_poll_in(data->target, p_char);
}

static void poll_out(const struct device *dev, unsigned char out_char)
{
	__ASSERT_NO_MSG(dev);

	const struct uart_async_adapter_data *data = dev->data;

	__ASSERT_NO_MSG(data);
	__ASSERT_NO_MSG(data->target);

	uart_poll_out(data->target, out_char);
}

static int err_check(const struct device *dev)
{
	__ASSERT_NO_MSG(dev);

	const struct uart_async_adapter_data *data = dev->data;

	__ASSERT_NO_MSG(data);
	__ASSERT_NO_MSG(data->target);

	return uart_err_check(data->target);
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int configure(const struct device *dev, const struct uart_config *cfg)
{
	__ASSERT_NO_MSG(dev);

	const struct uart_async_adapter_data *data = dev->data;

	__ASSERT_NO_MSG(data);
	__ASSERT_NO_MSG(data->target);

	return uart_configure(data->target, cfg);
}

static int config_get(const struct device *dev, struct uart_config *cfg)
{
	__ASSERT_NO_MSG(dev);

	const struct uart_async_adapter_data *data = dev->data;

	__ASSERT_NO_MSG(data);
	__ASSERT_NO_MSG(data->target);

	return uart_config_get(data->target, cfg);
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */


#ifdef CONFIG_UART_LINE_CTRL

static int line_ctrl_set(const struct device *dev, uint32_t ctrl, uint32_t val)
{
	__ASSERT_NO_MSG(dev);

	const struct uart_async_adapter_data *data = dev->data;

	__ASSERT_NO_MSG(data);
	__ASSERT_NO_MSG(data->target);

	return uart_line_ctrl_set(data->target, ctrl, val);
}

static int line_ctrl_get(const struct device *dev, uint32_t ctrl, uint32_t *val)
{
	__ASSERT_NO_MSG(dev);

	const struct uart_async_adapter_data *data = dev->data;

	__ASSERT_NO_MSG(data);
	__ASSERT_NO_MSG(data->target);

	return uart_line_ctrl_get(data->target, ctrl, val);
}

#endif /* CONFIG_UART_LINE_CTRL */

#ifdef CONFIG_UART_DRV_CMD

static int drv_cmd(const struct device *dev, uint32_t cmd, uint32_t p)
{
	__ASSERT_NO_MSG(dev);

	const struct uart_async_adapter_data *data = dev->data;

	__ASSERT_NO_MSG(data);
	__ASSERT_NO_MSG(data->target);

	return uart_drv_cmd(data->target, cmd, p);
}

#endif /* CONFIG_UART_DRV_CMD */

static inline void on_tx_ready(const struct device *dev, struct uart_async_adapter_data *data)
{
	k_spinlock_key_t key = k_spin_lock(&(data->lock));

	LOG_DBG("%s: Enter(%s) (left: %u)", __func__, dev->name, data->tx.size_left);
	if (data->tx.size_left) {
		__ASSERT_NO_MSG(data->tx.curr_buf);
		int ret;

		ret = uart_fifo_fill(data->target, data->tx.curr_buf, data->tx.size_left);
		LOG_DBG("Pushed %d characters", ret);
		if (ret <= 0) {
			LOG_ERR("Unexpected fifo fill err: %d", ret);
		} else {
			data->tx.curr_buf += ret;
			data->tx.size_left -= ret;
		}
	}

	k_spin_unlock(&(data->lock), key);

	LOG_DBG("%s: Exit", __func__);
}

static inline void on_tx_complete(const struct device *dev, struct uart_async_adapter_data *data)
{
	LOG_DBG("%s: Enter(%s)", __func__, dev->name);
	if (!data->tx.size_left) {
		struct uart_event event = {UART_TX_DONE};

		data->tx.enabled = false;
		uart_irq_tx_disable(data->target);

		k_spinlock_key_t key = k_spin_lock(&(data->lock));

		if (data->tx.buf) {
			__ASSERT_NO_MSG(data->tx.curr_buf);
			__ASSERT_NO_MSG(data->tx.curr_buf >= data->tx.buf);
			/* Transfer really finished */
			event.data.tx.buf = data->tx.buf;
			event.data.tx.len = data->tx.curr_buf - data->tx.buf;
			/* Clear any pending tx */
			data->tx.buf = NULL;
			data->tx.curr_buf = NULL;
			data->tx.size_left = 0;
		}

		k_spin_unlock(&(data->lock), key);

		if (event.data.tx.buf) {
			LOG_DBG("Notification: UART_TX_DONE (0x%x, size: %d)",
				(unsigned int)event.data.tx.buf, event.data.tx.len);
			user_callback(dev, &event);
		} else {
			LOG_ERR("TX buffer NULL on tx complete");
		}
	}
	LOG_DBG("%s: Exit", __func__);
}

static inline void on_rx_ready(const struct device *dev, struct uart_async_adapter_data *data)
{
	int ret;
	bool notify_now = false;

	LOG_DBG("%s: Enter (%s)", __func__, dev->name);
	if (data->rx.timeout != SYS_FOREVER_US) {
		k_timer_start(&data->rx.timeout_timer, K_USEC(data->rx.timeout), K_NO_WAIT);
	}
	do {
		k_spinlock_key_t key = k_spin_lock(&(data->lock));

		if (!data->rx.size_left) {
			notify_now = false;

			k_spin_unlock(&(data->lock), key);

			notify_rx_buffer(dev);
			switch_rx_buffer(dev, true);

			key = k_spin_lock(&(data->lock));
		}
		if (!data->rx.size_left) {
			/* Data received without buffer - dropping */
			uint8_t dummy;
			size_t cnt = 0;

			do {
				ret = uart_fifo_read(data->target, &dummy, 1);
				if (ret < 0) {
					LOG_ERR("Unexpected error on FIFO dropping: %d", ret);
					ret = 0;
				}
				cnt += ret;
			} while (ret);
			LOG_ERR("Data received without buffer prepared, dropped %d bytes", cnt);
		} else {
			ret = uart_fifo_read(data->target, data->rx.curr_buf, data->rx.size_left);
			LOG_DBG("Received %d characters", ret);
			if (ret < 0) {
				LOG_ERR("Unexpected error on FIFO read: %d", ret);
				ret = 0;
			}
			__ASSERT_NO_MSG(data->rx.size_left >= ret);
			data->rx.curr_buf += ret;
			data->rx.size_left -= ret;
			if (data->rx.timeout == 0) {
				notify_now = true;
			}
		}

		k_spin_unlock(&(data->lock), key);

	} while (ret);
	if (notify_now) {
		notify_rx_buffer(dev);
	}
	LOG_DBG("%s: Exit", __func__);
}

static inline void on_error(const struct device *dev,
			    struct uart_async_adapter_data *data,
			    int rx_err)
{
	struct uart_event event = {
		.type = UART_RX_STOPPED,
		.data.rx_stop.reason = rx_err
	};

	LOG_DBG("%s: Enter(%s)", __func__, dev->name);
	rx_disable(data->target);
	user_callback(dev, &event);
	LOG_DBG("%s: Exit", __func__);
}

static void uart_irq_handler(const struct device *target_dev, void *context)
{
	const struct device *dev = context;
	struct uart_async_adapter_data *data = access_dev_data(dev);

	__ASSERT(target_dev == data->target,
		"IRQ handler called with a context that seems uninitialized.");
	LOG_DBG("irq_handler: Enter");
	if (uart_irq_update(target_dev) && uart_irq_is_pending(target_dev)) {
		if (data->tx.enabled && uart_irq_tx_ready(target_dev)) {
			on_tx_ready(dev, data);
		}
		if (data->tx.enabled && uart_irq_tx_complete(target_dev)) {
			on_tx_complete(dev, data);
		}
		if (data->rx.enabled && uart_irq_rx_ready(target_dev)) {
			on_rx_ready(dev, data);
		}

		/* Check errors only after all the data is received from the device */
		int rx_err = uart_err_check(target_dev);

		if (rx_err < 0) {
			rx_err = 0;
		}
		if (data->rx.enabled && rx_err) {
			on_error(dev, data, rx_err);
		}
	}
	LOG_DBG("irq_handler: Exit");
}

const struct uart_driver_api uart_async_adapter_driver_api = {
	/* Async interface */
	.callback_set = callback_set,
	.tx = tx,
	.tx_abort = tx_abort,
	.rx_enable = rx_enable,
	.rx_buf_rsp = rx_buf_rsp,
	.rx_disable = rx_disable,

	/* Basic functions */
	.poll_in = poll_in,
	.poll_out = poll_out,
	.err_check = err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = configure,
	.config_get = config_get,
#endif

#ifdef CONFIG_UART_LINE_CTRL
	.line_ctrl_set = line_ctrl_set,
	.line_ctrl_get = line_ctrl_get
#endif

#ifdef CONFIG_UART_DRV_CMD
	.drv_cmd = drv_cmd
#endif
};


static void tx_timeout(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);

	(void)tx_abort(dev);
}

static void rx_timeout(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);

	notify_rx_buffer(dev);
}

void uart_async_adapter_init(const struct device *dev, const struct device *target)
{
	__ASSERT_NO_MSG(dev);
	__ASSERT_NO_MSG(dev->api);
	__ASSERT(dev->api == &uart_async_adapter_driver_api,
		"This is not uart_async_adapter device");

	struct uart_async_adapter_data *data = access_dev_data(dev);

	data->target = target;
	uart_irq_callback_user_data_set(data->target, uart_irq_handler, (void *)dev);

	k_timer_init(&data->tx.timeout_timer, tx_timeout, NULL);
	k_timer_user_data_set(&data->tx.timeout_timer, (void *)dev);
	k_timer_init(&data->rx.timeout_timer, rx_timeout, NULL);
	k_timer_user_data_set(&data->rx.timeout_timer, (void *)dev);

	dev->state->initialized = true;
}
