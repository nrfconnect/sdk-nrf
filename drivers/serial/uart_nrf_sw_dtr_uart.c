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

/* Low power uart structure. */
struct dtr_uart_data {

	/* Device structure */ // MARKUS TODO: This is us, should be a nicer way to get.
	const struct device *dev;

	/* Timer used for TX timeouting. */
	struct k_timer tx_timer;

	/* Current TX buffer. */
	const uint8_t *tx_buf;
	size_t tx_len;
	bool tx_started;

	bool app_rx_enable;
	bool rx_active;

	uart_callback_t user_callback;
	void *user_data;

	/* Semaphore used to wait for RX to be disabled */
	struct k_sem rx_disable_sem;

	/* DTR state: 0 = deasserted (UART inactive), 1 = asserted (UART active) */
	bool dtr_state;
	struct gpio_callback dtr_cb;

	/* Worker for processing DTR signal changes */
	struct k_work_delayable dtr_work;

	/* Worker for deactivating RI signal. */
	struct k_work_delayable ri_work;
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

static void tx_complete(struct dtr_uart_data *data)
{
	data->tx_started = false;
	data->tx_buf = NULL;
	data->tx_len = 0;
}

static void activate_tx(struct dtr_uart_data *data)
{
	const struct dtr_uart_config *config = get_dev_config(get_dev(data));

	if (data->tx_buf && data->tx_len > 0) {
		int err;

		data->tx_started = true;
		err = uart_tx(config->uart, data->tx_buf, data->tx_len, SYS_FOREVER_US);
		if (err) {
			LOG_ERR("TX: Not started (%d).", err);

			struct uart_event evt = {
				.type = UART_TX_ABORTED,
				.data.tx.buf = data->tx_buf,
				.data.tx.len = 0,
			};

			tx_complete(data);
			user_callback(get_dev(data), &evt);
		}
	}
}

static int deactivate_tx(struct dtr_uart_data *data)
{
	const struct dtr_uart_config *config = get_dev_config(get_dev(data));
	int err;

	// MARKUS TODO: Atomic.
	if (data->tx_buf && !data->tx_started) {
		LOG_DBG("TX: Abort - Before started.");

		struct uart_event evt = {
			.type = UART_TX_ABORTED,
			.data.tx.buf = data->tx_buf,
			.data.tx.len = 0,
		};
		tx_complete(data);
		user_callback(get_dev(data), &evt);
		return 0;
	}

	err = uart_tx_abort(config->uart);
	if (err == 0) {
		LOG_DBG("TX: Abort.");
	} else if (err == -EFAULT) {
		LOG_DBG("TX: Abort - No active transfer.");
	} else {
		/* We assume that UART_TX_ABORTED is sent. */
		LOG_ERR("TX: Abort (%d).", err);
	}

	return err;
}

static int deactivate_rx(struct dtr_uart_data *data)
{
	const struct dtr_uart_config *config = get_dev_config(get_dev(data));
	int err;

	if (!data->rx_active) {
		LOG_WRN("RX: Not active");
		return -EALREADY;
	}
	data->rx_active = false;
	err = uart_rx_disable(config->uart);
	if (err) {
		LOG_ERR("RX: Failed to disable (err: %d)", err);
	}

	return err;
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
		LOG_DBG("UART powered on");
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
		LOG_DBG("UART powered off");
	}
	return err;
}

/**************************** WORKERS **********************************/
static void ri_work_fn(struct k_work *work)
{
	struct k_work_delayable *delayed_work = CONTAINER_OF(work, struct k_work_delayable, work);
	struct dtr_uart_data *data =
		CONTAINER_OF(delayed_work, struct dtr_uart_data, ri_work);

	gpio_pin_set_dt(&get_dev_config(get_dev(data))->ri_gpio, 0);
}

static void ri_start(struct dtr_uart_data *data)
{
	gpio_pin_set_dt(&get_dev_config(get_dev(data))->ri_gpio, 1);
	k_work_schedule(&data->ri_work, K_MSEC(1000));
}

static void uart_dtr_input_gpio_callback(const struct device *port, struct gpio_callback *cb,
					 uint32_t pins)
{
	struct dtr_uart_data *data = CONTAINER_OF(cb, struct dtr_uart_data, dtr_cb);

	k_work_reschedule(&data->dtr_work, K_MSEC(10)); // MARKUS TODO: Debounce time?
}

static void dtr_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct dtr_uart_data *data = CONTAINER_OF(dwork, struct dtr_uart_data, dtr_work);
	const struct dtr_uart_config *cfg = get_dev_config(data->dev);
	bool asserted = gpio_pin_get_dt(&cfg->dtr_gpio);
	int err;

	if (data->dtr_state == asserted) {
		LOG_WRN("DTR is already %s, ignoring event", asserted ? "asserted" : "deasserted");
		return;
	}

	data->dtr_state = asserted;

	if (asserted) {
		/* Stop RI signal. */
		k_work_cancel_delayable(&data->ri_work);
		gpio_pin_set_dt(&get_dev_config(get_dev(data))->ri_gpio, 0);

		/* Power on UART module */
		err = power_on_uart(data);
		if (err) {
			LOG_ERR("Failed to resume UART device: %d", err);
		}

		activate_rx(data);
		activate_tx(data);
	} else {
		deactivate_tx(data);

		k_sem_reset(&data->rx_disable_sem);
		err = deactivate_rx(data);
		if (err == 0) {
			/* Wait for RX to be fully disabled, before powering UART down. */
			k_sem_take(&data->rx_disable_sem, K_MSEC(100));
		}
		power_off_uart(data);
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
		LOG_DBG("TX: done");
		tx_complete(data);
		user_callback(dev, evt);
		break;
	case UART_TX_ABORTED:
		LOG_DBG("TX: abort");
		tx_complete(data);
		user_callback(dev, evt);
		break;
	case UART_RX_RDY:
		LOG_DBG("RX: Ready buf:%p, offset: %d,len: %d", (void *)evt->data.rx.buf,
			evt->data.rx.offset, evt->data.rx.len);
		user_callback(dev, evt);
		break;

	case UART_RX_BUF_REQUEST:
		LOG_DBG("UART_RX_BUF_REQUEST");
		user_callback(dev, evt);
		break;

	case UART_RX_BUF_RELEASED:
		LOG_DBG("Rx buf released %p", (void *)evt->data.rx_buf.buf);
		user_callback(dev, evt);
		break;

	case UART_RX_DISABLED: {
		LOG_DBG("UART_RX_DISABLED %d", data->dtr_state);
		/* When RX disabled because of DTR down, we handle it ourselves. */
		if (data->dtr_state && data->app_rx_enable) {
			data->app_rx_enable = false;
			user_callback(dev, evt);
		}

		if (!data->dtr_state) {
			/* RX disabled because of DTR down. */
			k_sem_give(&data->rx_disable_sem);
		}
		break;
	}
	case UART_RX_STOPPED:
		LOG_DBG("Rx stopped");
		if (data->dtr_state && data->app_rx_enable) {
			user_callback(dev, evt);
		}
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

	k_work_init_delayable(&data->dtr_work, dtr_work_handler);
	k_work_init_delayable(&data->ri_work, ri_work_fn);

	k_sem_init(&data->rx_disable_sem, 0, 1);

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
	/* MARKUS TODO: -> Possibly through pin_trigger_enable, which is called in interrupt set.
	 * What if we just don't disable interrupt when shutting down? */
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

	LOG_DBG("api_tx: %.*s", len, buf);

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

	/* Start RI pulse. */
	ri_start(data);

	/* Buffer the data until DTR is down. */
	return 0;
}

static int api_tx_abort(const struct device *dev)
{
	struct dtr_uart_data *data = get_dev_data(dev);

	return deactivate_tx(data);
}

/* Application calls this when it is ready to receive data. */
static int api_rx_enable(const struct device *dev, uint8_t *buf, size_t len, int32_t timeout)
{
	const struct dtr_uart_config *config = get_dev_config(dev);
	struct dtr_uart_data *data = get_dev_data(dev);

	LOG_DBG("api_rx_enable: %p, %zu", (void *)buf, len);

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
	struct dtr_uart_data *data = get_dev_data(dev);

	LOG_DBG("api_rx_disable");

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
