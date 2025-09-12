/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/pm/device.h>

/*
 * DTR (Data Terminal Ready) Logic:
 *
 * This driver implements DTR flow control where DTR input levels directly
 * correspond to DTR assertion/deassertion events:
 *
 * DTR Input Level 0 → DTR_DEASSERTED → UART inactive (powered down)
 * DTR Input Level 1 → DTR_ASSERTED   → UART active (powered up and ready)
 *
 * Internal state representation (matches input level):
 * - dtr_state = 0: DTR deasserted, UART inactive
 * - dtr_state = 1: DTR asserted, UART active
 */

LOG_MODULE_REGISTER(dtr_uart, CONFIG_NRF_SW_DTR_UART_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_sw_dtr_uart

enum dtr_uart_event {
	/* RX disabled event. */
	DTR_UART_EVENT_RX_DISABLED,

	/* DTE event: No traffic between DTE and DCE for a configured timeout -> DTR deasserted. */
	DTR_UART_EVENT_IDLE,
};

static const char *dtr_event_str(enum dtr_uart_event event)
{
	switch (event) {
	case DTR_UART_EVENT_RX_DISABLED:
		return "RX DISABLED";
	case DTR_UART_EVENT_IDLE:
		return "DTR IDLE";
	}

	return "";
}

enum dtr_timeout_event {

	/* No timeout pending. */
	DTR_TIMEOUT_NO_TIMEOUT = 0,

	/* RI timeout. */
	DTR_TIMEOUT_RI,

	/* DTR activation timeout. */
	DTR_TIMEOUT_DTR_ACTIVATION,

	/* DTR idle timeout. */
	DTR_TIMEOUT_DTR_IDLE,
};

static const char *dtr_timeout_str(enum dtr_timeout_event timeout)
{
	switch (timeout) {
	case DTR_TIMEOUT_NO_TIMEOUT:
		return "NO TIMEOUT";
	case DTR_TIMEOUT_RI:
		return "RI TIMEOUT";
	case DTR_TIMEOUT_DTR_ACTIVATION:
		return "DTR ACTIVATION TIMEOUT";
	case DTR_TIMEOUT_DTR_IDLE:
		return "DTR IDLE TIMEOUT";
	}

	return "";
}

/* Low power uart structure. */
struct dtr_uart_data {

	/* Device structure */ // MARKUS TODO: This is us, should be a nicer way to get.
	const struct device *dev;

	/* Timer used for TX timeouting. */
	struct k_timer tx_timer;

	/* Current TX buffer. */
	const uint8_t *tx_buf;
	size_t tx_len;

	bool app_rx_enable;
	bool rx_active;

	uart_callback_t user_callback;
	void *user_data;

	/* DTR state: 0 = deasserted (UART inactive), 1 = asserted (UART active) */
	bool dtr_state;
	struct gpio_callback dtr_cb;

	/* Worker and message queue for DTR events */
	struct k_work event_queue_work;
	struct k_msgq event_queue;
	enum dtr_uart_event event_queue_buf[8];
	struct k_work_delayable dtr_work;

	/* One timeout event can run at a time */
	enum dtr_timeout_event dtr_timeout_event;
	struct k_work_delayable dtr_timeout_work;
};

/* Forward declarations. */
static void user_callback(const struct device *dev, struct uart_event *evt);

/* Configuration structured. */
struct dtr_uart_config {
	/* Physical UART device */
	const struct device *uart;
	struct gpio_dt_spec dtr_gpio;
	struct gpio_dt_spec ri_gpio;
};

static inline struct dtr_uart_data *get_dev_data(const struct device *dev)
{
	return dev->data;
}

static inline const struct dtr_uart_config *get_dev_config(const struct device *dev)
{
	return dev->config;
}

static inline const struct device *get_dev(struct dtr_uart_data *data)
{
	return data->dev;
}

static void activate_tx(struct dtr_uart_data *data)
{
	const struct dtr_uart_config *config = get_dev_config(get_dev(data));

	if (data->tx_buf && data->tx_len > 0) {
		int err;

		err = uart_tx(config->uart, data->tx_buf, data->tx_len, SYS_FOREVER_US);
		if (err < 0) {
			LOG_ERR("TX: Not started (error: %d)", err);
			// tx_complete(data);
		}
	}
}

static void deactivate_tx(struct dtr_uart_data *data)
{
	const struct dtr_uart_config *config = get_dev_config(get_dev(data));

	// MARKUS TODO: This needs to take care of scenario where we have not yet sent at all.
	if (data->tx_buf) {
		int err;

		// MARKUS TODO: We must wait for the abort to take place before we disable UART.
		err = uart_tx_abort(config->uart);
		if (err < 0) {
			LOG_ERR("TX: Abort (error: %d)", err);
			// tx_complete(data);
		}
	}
}

static void tx_complete(struct dtr_uart_data *data)
{
	data->tx_buf = NULL;
	data->tx_len = 0;
}

static void deactivate_rx(struct dtr_uart_data *data)
{
	const struct dtr_uart_config *config = get_dev_config(get_dev(data));
	int err;

	if (!data->rx_active) {
		LOG_WRN("RX: Not active");
		return;
	}
	data->rx_active = false;
	/* abort rx */
	err = uart_rx_disable(config->uart);
	if (err) {
		LOG_ERR("RX: Failed to disable (err: %d)", err);
	}
}

static void activate_rx(struct dtr_uart_data *data)
{
	if (data->rx_active) {
		LOG_WRN("RX: Already active");
		return;
	}

	if (!data->app_rx_enable) {
		LOG_WRN("RX: Not enabled by app");
		return;
	}

	struct uart_event evt = {
		.type = UART_RX_BUF_REQUEST,
	};
	user_callback(get_dev(data), &evt);
}

static void event_queue_delegate(struct dtr_uart_data *data, enum dtr_uart_event evt)
{
	if (k_msgq_put(&data->event_queue, &evt, K_NO_WAIT)) {
		__ASSERT(false, "Failed to enqueue event");
	}
	k_work_submit(&data->event_queue_work);
}

/* Timeout for scheduled task. */
static bool set_dtr_timeout_event(struct dtr_uart_data *data, enum dtr_timeout_event timeout,
				  k_timeout_t duration)
{
	if (data->dtr_timeout_event == timeout) {
		LOG_INF("DTR timeout event already set to same value");
		k_work_reschedule(&data->dtr_timeout_work, duration);
		return true;
	} else if (data->dtr_timeout_event == DTR_TIMEOUT_NO_TIMEOUT) {
		LOG_INF("DTR timeout event set to %s", dtr_timeout_str(timeout));
		data->dtr_timeout_event = timeout;
		k_work_schedule(&data->dtr_timeout_work, duration);
		return true;
	}

	LOG_WRN("DTR timeout event already set");
	return false;
}

static void cancel_dtr_timeout_event(struct dtr_uart_data *data)
{
	data->dtr_timeout_event = DTR_TIMEOUT_NO_TIMEOUT;
	k_work_cancel_delayable(&data->dtr_timeout_work);
}

static int power_on_uart(struct dtr_uart_data *data)
{
	const struct dtr_uart_config *config = get_dev_config(get_dev(data));
	enum pm_device_state state = PM_DEVICE_STATE_OFF;

	int err = pm_device_state_get(config->uart, &state);
	if (err) {
		LOG_ERR("Failed to get PM device state: %d", err);
		return err;
	}
	if (state != PM_DEVICE_STATE_ACTIVE) {
		/* Power on UART module */
		err = pm_device_action_run(config->uart, PM_DEVICE_ACTION_RESUME);
		if (err) {
			LOG_ERR("Failed to resume UART device: %d", err);
		}
	}

	return err;
}

static int power_off_uart(struct dtr_uart_data *data)
{
	const struct dtr_uart_config *config = get_dev_config(get_dev(data));
	enum pm_device_state state = PM_DEVICE_STATE_ACTIVE;

	int err = pm_device_state_get(config->uart, &state);
	if (err) {
		LOG_ERR("Failed to get PM device state: %d", err);
		return err;
	}
	if (state != PM_DEVICE_STATE_SUSPENDED) {
		/* Power off UART module */
		err = pm_device_action_run(config->uart, PM_DEVICE_ACTION_SUSPEND);
		if (err) {
			LOG_ERR("Failed to suspend UART device: %d", err);
		}
	}
	return err;
}

static void event_rx_disabled_work(struct dtr_uart_data *data)
{
	data->rx_active = false;
	if (data->dtr_state == 1) {
		LOG_INF("RX disabled event in state %d, not suspending", data->dtr_state);
		return;
	}

	/* Power off UART module */
	const int err = power_off_uart(data);
	if (err) {
		LOG_ERR("Failed to suspend UART device: %d", err);
	}
}

/**************************** WORKERS **********************************/
static void event_queue_work_fn(struct k_work *work)
{
	struct dtr_uart_data *data = CONTAINER_OF(work, struct dtr_uart_data, event_queue_work);
	enum dtr_uart_event evt;

	while (k_msgq_get(&data->event_queue, &evt, K_NO_WAIT) == 0) {
		LOG_INF("DTR event \"%s\" in state %d", dtr_event_str(evt), data->dtr_state);

		switch (evt) {
		case DTR_UART_EVENT_IDLE:
			break;
		case DTR_UART_EVENT_RX_DISABLED:
			event_rx_disabled_work(data);
			break;
		default:
			LOG_WRN("Unknown event: %d", evt);
			break;
		}

		LOG_INF("DTR event \"%s\" done, new state %d", dtr_event_str(evt), data->dtr_state);
	}
}

static void dtr_timeout_work_fn(struct k_work *work)
{
	struct k_work_delayable *delayed_work = CONTAINER_OF(work, struct k_work_delayable, work);
	struct dtr_uart_data *data =
		CONTAINER_OF(delayed_work, struct dtr_uart_data, dtr_timeout_work);

	LOG_INF("DTR timeout event \"%s\" in state %d", dtr_timeout_str(data->dtr_timeout_event),
		data->dtr_state);

	switch (data->dtr_timeout_event) {
	case DTR_TIMEOUT_RI:
		gpio_pin_set_dt(&get_dev_config(get_dev(data))->ri_gpio, 0);
		break;
	case DTR_TIMEOUT_DTR_ACTIVATION:
		/* TODO: handle error */
	case DTR_TIMEOUT_DTR_IDLE:
		event_queue_delegate(data, DTR_UART_EVENT_IDLE);
		break;
	default:
		LOG_WRN("Unknown timeout event: %d", data->dtr_timeout_event);
		break;
	}

	data->dtr_timeout_event = DTR_TIMEOUT_NO_TIMEOUT;
}

static void ri_start(struct dtr_uart_data *data)
{
	gpio_pin_set_dt(&get_dev_config(get_dev(data))->ri_gpio, 1);
	set_dtr_timeout_event(data, DTR_TIMEOUT_RI, K_MSEC(1000));
}

/******************************* GPIO ********************************/

static void uart_dtr_input_gpio_callback(const struct device *port, struct gpio_callback *cb,
					 uint32_t pins)
{
	struct dtr_uart_data *data = CONTAINER_OF(cb, struct dtr_uart_data, dtr_cb);

	k_work_reschedule(&data->dtr_work, K_MSEC(10));
}

static void dtr_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct dtr_uart_data *data = CONTAINER_OF(dwork, struct dtr_uart_data, dtr_work);
	const struct dtr_uart_config *cfg = get_dev_config(data->dev);
	bool asserted = gpio_pin_get_dt(&cfg->dtr_gpio);

	if (data->dtr_state == asserted) {
		LOG_WRN("DTR is already %s, ignoring event", asserted ? "asserted" : "deasserted");
		return;
	}

	data->dtr_state = asserted;

	if (asserted) {
		cancel_dtr_timeout_event(data);
		gpio_pin_set_dt(&get_dev_config(get_dev(data))->ri_gpio, 0);

		/* Power on UART module */
		const int err = power_on_uart(data);

		if (err) {
			LOG_ERR("Failed to resume UART device: %d", err);
		}

		activate_rx(data);
		activate_tx(data);
	} else {
		deactivate_tx(data);
		deactivate_rx(data);

		/* Wait for UART to be disabled, before powering it down. */
	}
}

/****************************** UART ********************************/

static void user_callback(const struct device *dev, struct uart_event *evt)
{
	const struct dtr_uart_data *data = get_dev_data(dev);

	if (data->user_callback) {
		data->user_callback(dev, evt, data->user_data);
	}
}

static void uart_callback(const struct device *uart, struct uart_event *evt, void *user_data)
{
	struct device *dev = user_data;
	struct dtr_uart_data *data = get_dev_data(dev);

	switch (evt->type) {
	case UART_TX_DONE:
		LOG_INF("TX: done");
		tx_complete(data);
		user_callback(dev, evt);
		break;
	case UART_TX_ABORTED:
		LOG_INF("TX: abort");
		tx_complete(data);
		user_callback(dev, evt);
		break;
	case UART_RX_RDY:
		LOG_INF("RX: Ready buf:%p, offset: %d,len: %d", (void *)evt->data.rx.buf,
			evt->data.rx.offset, evt->data.rx.len);
		user_callback(dev, evt);
		break;

	case UART_RX_BUF_REQUEST:
		LOG_INF("UART_RX_BUF_REQUEST");
		user_callback(dev, evt);
		break;

	case UART_RX_BUF_RELEASED:
		LOG_INF("Rx buf released %p", (void *)evt->data.rx_buf.buf);
		user_callback(dev, evt);
		break;

	case UART_RX_DISABLED: {
		LOG_INF("UART_RX_DISABLED %d", data->dtr_state);
		/* When RX disabled because of DTR down, we handle it ourselves. */
		if (data->dtr_state && data->app_rx_enable) {
			data->app_rx_enable = false;
			user_callback(dev, evt);
		}

		event_queue_delegate(data, DTR_UART_EVENT_RX_DISABLED);
		break;
	}
	case UART_RX_STOPPED:
		LOG_INF("Rx stopped");
		if (data->dtr_state && data->app_rx_enable) {
			user_callback(dev, evt);
		}
		event_queue_delegate(data, DTR_UART_EVENT_RX_DISABLED);
		break;
	}
}

static int dtr_uart_init(const struct device *dev)
{
	struct dtr_uart_data *data = get_dev_data(dev);
	const struct dtr_uart_config *cfg = get_dev_config(dev);
	int err;
	data->dev = dev;

	if (!device_is_ready(cfg->uart)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	/* DTR */
	if (!gpio_is_ready_dt(&cfg->dtr_gpio)) {
		LOG_ERR("DTR GPIO not ready");
		return -ENODEV;
	}

	/* Configure DTR GPIO as input */
	err = gpio_pin_configure_dt(&cfg->dtr_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure DTR GPIO: %d", err);
		return err;
	}

	/* RI */
	if (!gpio_is_ready_dt(&cfg->ri_gpio)) {
		LOG_ERR("RI GPIO not ready");
		return -ENODEV;
	}
	/* Configure RI GPIO as output, initially inactive */
	err = gpio_pin_configure_dt(&cfg->ri_gpio, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure RI GPIO: %d", err);
		return err;
	}

	k_msgq_init(&data->event_queue, (char *)&data->event_queue_buf, sizeof(enum dtr_uart_event),
		    sizeof(data->event_queue_buf) / sizeof(enum dtr_uart_event));
	k_work_init(&data->event_queue_work, event_queue_work_fn);
	k_work_init_delayable(&data->dtr_work, dtr_work_handler);
	k_work_init_delayable(&data->dtr_timeout_work, dtr_timeout_work_fn);

	/* Read initial DTR state */
	int initial_dtr_state = gpio_pin_get_dt(&cfg->dtr_gpio);

	if (initial_dtr_state < 0) {
		LOG_ERR("Failed to read initial DTR state: %d", initial_dtr_state);
		return initial_dtr_state;
	}
	/* Map GPIO input level directly to DTR state:
	 * Input level 0 → DTR deasserted (dtr_state = 0, UART inactive)
	 * Input level 1 → DTR asserted (dtr_state = 1, UART active)
	 */
	data->dtr_state = initial_dtr_state;

	/* TODO: How to do this from Zephyr API: Set sense to wake the DCE from possible shutdown */
	/* nrf_gpio_cfg_sense_set(data->dtr_pin, NRF_GPIO_PIN_SENSE_LOW); */

	/* Set up GPIO interrupt for DTR changes */
	gpio_init_callback(&data->dtr_cb, uart_dtr_input_gpio_callback, BIT(cfg->dtr_gpio.pin));

	err = gpio_add_callback(cfg->dtr_gpio.port, &data->dtr_cb);
	if (err < 0) {
		LOG_ERR("Failed to add DTR GPIO callback: %d", err);
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&cfg->dtr_gpio, GPIO_INT_EDGE_BOTH);
	if (err < 0) {
		LOG_ERR("Failed to configure DTR GPIO interrupt: %d", err);
		return err;
	}

	err = uart_callback_set(cfg->uart, uart_callback, (void *)dev);
	if (err < 0) {
		return -EINVAL;
	}

	LOG_DBG("DTR UART initialized, initial DTR state: %d", data->dtr_state);

	return 0;
}

/**************************** API ********************************/

static int api_callback_set(const struct device *dev, uart_callback_t callback, void *user_data)
{
	struct dtr_uart_data *data = get_dev_data(dev);

	data->user_callback = callback;
	data->user_data = user_data;

	return 0;
}

static int api_tx(const struct device *dev, const uint8_t *buf, size_t len, int32_t timeout)
{
	const struct dtr_uart_config *config = get_dev_config(dev);

	if (buf == NULL || len == 0) {
		return 0;
	}
	LOG_INF("api_tx: %.*s", len, buf);

	struct dtr_uart_data *data = get_dev_data(dev);

	if (data->tx_buf) {
		LOG_ERR("TX scheduled");
		return -EBUSY;
	}

	if (data->dtr_state == 1) {
		return uart_tx(config->uart, buf, len, timeout);
	}
	data->tx_buf = buf;
	data->tx_len = len;

	/* DCE - Start RI pulse. */
	ri_start(data);

	/* Buffer the data until DTR is down. */
	return 0;
}

static int api_tx_abort(const struct device *dev)
{
	return -ENOTSUP;
}

/* Application calls this when it is ready to receive data. */
static int api_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	LOG_INF("api_rx_enable: %p, %zu", (void *)buf, len);

	const struct dtr_uart_config *config = get_dev_config(dev);
	struct dtr_uart_data *data = get_dev_data(dev);

	if (data->app_rx_enable) {
		LOG_ERR("RX already enabled");
		return -EBUSY;
	}

	data->app_rx_enable = true;
	if (!data->dtr_state) {
		struct uart_event evt = {
			.type = UART_RX_BUF_RELEASED,
			.data.rx_buf.buf = buf,
		};
		user_callback(dev, &evt);
		return 0;
	}
	data->rx_active = true;
	return uart_rx_enable(config->uart, buf, len, 2000);
}

static int api_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	const struct dtr_uart_config *config = get_dev_config(dev);
	struct dtr_uart_data *data = get_dev_data(dev);

	LOG_DBG("api_rx_buf_rsp: %p, %zu", (void *)buf, len);

	if (!data->dtr_state) {
		goto release;
	}

	if (!data->app_rx_enable) {
		goto release;
	}

	if (!data->rx_active) {
		data->rx_active = true;
		uart_rx_enable(config->uart, buf, len, 2000);
		return 0;
	}

	return uart_rx_buf_rsp(config->uart, buf, len);
release:
	struct uart_event evt = {
		.type = UART_RX_BUF_RELEASED,
		.data.rx_buf.buf = buf,
	};
	user_callback(dev, &evt);
	return 0;
}

static int api_rx_disable(const struct device *dev)
{
	LOG_INF("api_rx_disable");
	struct dtr_uart_data *data = get_dev_data(dev);

	if (!data->app_rx_enable) {
		LOG_ERR("RX not enabled");
		return -EINVAL;
	}

	data->app_rx_enable = false;
	deactivate_rx(data);
	return 0;
}

static int api_err_check(const struct device *dev)
{
	const struct dtr_uart_config *config = get_dev_config(dev);

	return uart_err_check(config->uart);
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int api_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct dtr_uart_config *config = get_dev_config(dev);

	return uart_configure(config->uart, cfg);
}

static int api_config_get(const struct device *dev, struct uart_config *cfg)
{
	const struct dtr_uart_config *config = get_dev_config(dev);

	return uart_config_get(config->uart, cfg);
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

/* Power management */
#ifdef CONFIG_PM_DEVICE
static int dtr_uart_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		LOG_DBG("PM SUSPEND - Disabling UART");
		/* TODO */
		break;
	case PM_DEVICE_ACTION_RESUME:
		LOG_DBG("PM RESUME - Enabling UART");
		/* TODO */
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static const struct uart_driver_api dtr_uart_api = {
	.callback_set = api_callback_set,
	.tx = api_tx,
	.tx_abort = api_tx_abort,
	.rx_enable = api_rx_enable,
	.rx_buf_rsp = api_rx_buf_rsp,
	.rx_disable = api_rx_disable,
	.err_check = api_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = api_configure,
	.config_get = api_config_get,
#endif
};

/* Device macro */
#define DTR_UART_INIT(n)                                                                           \
	static const struct dtr_uart_config dtr_uart_config_##n = {                                \
		.dtr_gpio = GPIO_DT_SPEC_INST_GET(n, dtr_gpios),                                   \
		.ri_gpio = GPIO_DT_SPEC_INST_GET(n, ri_gpios),                                     \
		.uart = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(n))),                                  \
	};                                                                                         \
	static struct dtr_uart_data dtr_uart_data_##n;                                             \
	PM_DEVICE_DT_INST_DEFINE(n, dtr_uart_pm_action);                                           \
	DEVICE_DT_INST_DEFINE(n, dtr_uart_init, PM_DEVICE_DT_INST_GET(n), &dtr_uart_data_##n,      \
			      &dtr_uart_config_##n, POST_KERNEL, 51, &dtr_uart_api);

DT_INST_FOREACH_STATUS_OKAY(DTR_UART_INIT)
