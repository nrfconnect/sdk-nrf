/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>
#include <nrfx_gpiote.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/onoff.h>
#include <zephyr/pm/device.h>

LOG_MODULE_REGISTER(dtr_uart, CONFIG_NRF_SW_DTR_UART_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_sw_dtr_uart

/* States. */
enum dtr_state {

	/* DTR is up and we are disabled. */
	DTR_UP,

	/* DTR is pulled down to activate UART. */
	DTR_UP_TO_DOWN,

	/* DTR is down and we are active. */
	DTR_DOWN,

	/* DTR is pulled up to disable UART. */
	DTR_DOWN_TO_UP,
};

static const char *dtr_state_str(enum dtr_state state)
{
	switch (state) {
	case DTR_UP:
		return "UP";
	case DTR_UP_TO_DOWN:
		return "UP to DOWN";
	case DTR_DOWN:
		return "DOWN";
	case DTR_DOWN_TO_UP:
		return "DOWN to UP";
	}

	return "";
}

enum dtr_uart_event {

	/* DTR is up DTE/DCE will be be disabled. */
	DTR_UART_EVENT_UP = 0,

	/* DTR is down, DTE/DCE will be activated. */
	DTR_UART_EVENT_DOWN,

	/* RX buffer has been received. */
	DTR_UART_EVENT_RX_BUF_RECEIVED,

	/* RX disabled event. */
	DTR_UART_EVENT_RX_DISABLED,

	/* DTE event: DCE is ready or sufficient time has passed to assume it is. */
	DTR_UART_EVENT_DCE_READY,

	/* DTE event: No traffic between DTE and DCE for a configured timeout -> DTR UP. */
	DTR_UART_EVENT_IDLE,
};

static const char *dtr_event_str(enum dtr_uart_event event)
{
	switch (event) {
	case DTR_UART_EVENT_UP:
		return "DTR UP";
	case DTR_UART_EVENT_DOWN:
		return "DTR DOWN";
	case DTR_UART_EVENT_RX_BUF_RECEIVED:
		return "RX BUF RECEIVED";
	case DTR_UART_EVENT_RX_DISABLED:
		return "RX DISABLED";
	case DTR_UART_EVENT_DCE_READY:
		return "DCE READY";
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

	/* Physical UART device */
	const struct device *uart;

	/* Has the application called rx_enable?*/
	bool app_rx_enable;

	/* Data Terminal Equipment role. */
	bool dte_role;

	/* Data Terminal Ready pin. */
	nrfx_gpiote_pin_t dtr_pin;

	/* DTR pin for DCE uses toggle on high-to-low, instead of level trigger */
	bool dtr_pin_toggle;

	/* Debounce time for DTR pin. */
	uint32_t dtr_pin_debounce;

	/* Input pin trigger scheduled due to debounce. */
	nrfx_gpiote_trigger_t input_pin_trigger;

	/* Ring Indicator pin. */
	nrfx_gpiote_pin_t ri_pin;

	/* Timer used for TX timeouting. */
	struct k_timer tx_timer;

	/* Current TX buffer. */
	const uint8_t *tx_buf;
	size_t tx_len;

	/* Current RX buffer. */
	uint8_t *rx_buf;
	size_t rx_len;
	bool rx_buf_requested;

	uart_callback_t user_callback;
	void *user_data;

	/* DTR state */
	enum dtr_state dtr_state;

	/* Worker and message queue for DTR events */
	struct k_work event_queue_work;
	struct k_msgq event_queue;
	enum dtr_uart_event event_queue_buf[8];
	struct k_work dtr_event_work;

	/* One timeout event can run at a time */
	enum dtr_timeout_event dtr_timeout_event;
	struct k_work_delayable dtr_timeout_work;

	/* Delayed activation for input trigger. */
	struct k_work_delayable input_trigger_work;
};

/* Forward declarations. */
static void user_callback(const struct device *dev, struct uart_event *evt);

/* Configuration structured. */
struct dtr_uart_config {
	bool dte_role;
	nrfx_gpiote_pin_t dtr_pin;
	nrfx_gpiote_pin_t ri_pin;
	bool dtr_pin_toggle;
	uint32_t dtr_pin_debounce;
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

#define GPIOTE_NODE(gpio_node) DT_PHANDLE(gpio_node, gpiote_instance)
#define GPIOTE_INST_AND_COMMA(gpio_node) \
	IF_ENABLED(DT_NODE_HAS_PROP(gpio_node, gpiote_instance), ( \
	[DT_PROP(gpio_node, port)] = \
		NRFX_GPIOTE_INSTANCE(DT_PROP(GPIOTE_NODE(gpio_node), instance)),))

static const nrfx_gpiote_t *get_gpiote(nrfx_gpiote_pin_t pin)
{
	static const nrfx_gpiote_t gpiote[GPIO_COUNT] = {
		DT_FOREACH_STATUS_OKAY(nordic_nrf_gpio, GPIOTE_INST_AND_COMMA)
	};

	return &gpiote[pin >> 5];
}

static void dtr_down(struct dtr_uart_data *data)
{
	LOG_INF("dtr_down");
	nrf_gpio_pin_clear(data->dtr_pin);
}

static void dtr_up(struct dtr_uart_data *data)
{
	LOG_INF("dtr_up");
	nrf_gpio_pin_set(data->dtr_pin);
}

static void activate_tx(struct dtr_uart_data *data)
{
	if (data->tx_buf && data->tx_len > 0) {
		int err;

		err = uart_tx(data->uart, data->tx_buf, data->tx_len, SYS_FOREVER_US);
		if (err < 0) {
			LOG_ERR("TX: Not started (error: %d)", err);
			// tx_complete(data);
		}
	}
}

static void deactivate_tx(struct dtr_uart_data *data)
{
	// MARKUS TODO: This needs to take care of scenario where we have not yet sent at all.
	if (data->tx_buf) {
		int err;

		// MARKUS TODO: We must wait for the abort to take place before we disable UART.
		err = uart_tx_abort(data->uart);
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
	int err;

	/* abort rx */
	err = uart_rx_disable(data->uart);
	if (err) {
		LOG_ERR("RX: Failed to disable (err: %d)", err);
	}
}

static void request_rx_buffer(struct dtr_uart_data *data)
{
	if (data->rx_buf == NULL) {
		LOG_INF("RX: No buffer to enable RX -> Request buffer");

		struct uart_event evt = {
 			.type = UART_RX_BUF_REQUEST,
 		};

		/* Buffer should be immediately received. */
		data->rx_buf_requested = true;
		user_callback(get_dev(data), &evt);
		data->rx_buf_requested = false;
	}

	if (data->rx_buf) {
		LOG_INF("RX: Buffer is ready");
	} else {
		// MARKUS TODO: Try again later?
	}
}

static void activate_rx(struct dtr_uart_data *data)
{
	int err;

	__ASSERT(data->rx_buf != NULL && data->rx_len > 0, "RX: No buffer to enable RX");

	// MARKUS TODO: This should use events for retrial.
	err = uart_rx_enable(data->uart, data->rx_buf, data->rx_len, 2000);
	if (err < 0) {
		LOG_ERR("RX: Enabling failed (err: %d)", err);
		if (err == -EBUSY) {
			k_sleep(K_MSEC(10));
			err = uart_rx_enable(data->uart, data->rx_buf, data->rx_len, 2000);
		}
		__ASSERT(err == 0 || err == -EALREADY, "RX: Enabling failed (err:%d)", err);
	}

	data->rx_buf = NULL;
	data->rx_len = 0;
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

static void notify_idle(struct dtr_uart_data *data)
{
	if (IS_ENABLED(CONFIG_NRF_SW_DTR_UART_DTE_IDLE) && data->dte_role &&
	    data->dtr_state == DTR_DOWN) {
		set_dtr_timeout_event(data, DTR_TIMEOUT_DTR_IDLE,
				      K_MSEC(CONFIG_NRF_SW_DTR_UART_DTE_IDLE_TIMEOUT_MS));
	}
}

static int power_on_uart(struct dtr_uart_data *data)
{
	enum pm_device_state state = PM_DEVICE_STATE_OFF;

	int err = pm_device_state_get(data->uart, &state);
	if (err) {
		LOG_ERR("Failed to get PM device state: %d", err);
		return err;
	}
	if (state != PM_DEVICE_STATE_ACTIVE) {
		/* Power on UART module */
		err = pm_device_action_run(data->uart, PM_DEVICE_ACTION_RESUME);
		if (err) {
			LOG_ERR("Failed to resume UART device: %d", err);
		}
	}

	return err;
}

static int power_off_uart(struct dtr_uart_data *data)
{
	enum pm_device_state state = PM_DEVICE_STATE_ACTIVE;

	int err = pm_device_state_get(data->uart, &state);
	if (err) {
		LOG_ERR("Failed to get PM device state: %d", err);
		return err;
	}
	if (state != PM_DEVICE_STATE_SUSPENDED) {
		/* Power off UART module */
		err = pm_device_action_run(data->uart, PM_DEVICE_ACTION_SUSPEND);
		if (err) {
			LOG_ERR("Failed to suspend UART device: %d", err);
		}
	}
	return err;
}

static void event_down_work(struct dtr_uart_data *data)
{
	if (data->dtr_state == DTR_DOWN || data->dtr_state == DTR_UP_TO_DOWN) {
		LOG_WRN("DTR is already %s, ignoring event", "down");
		return;
	}

	data->dtr_state = DTR_UP_TO_DOWN;

	if (!data->dte_role) {
		/* DCE - stop RI */
		cancel_dtr_timeout_event(data);
		nrf_gpio_pin_clear(data->ri_pin);
	}

	/* Power on UART module */
	const int err = power_on_uart(data);
	if (err) {
		LOG_ERR("Failed to resume UART device: %d", err);
	}

	request_rx_buffer(data);
	activate_rx(data);

	if (data->dte_role) {
		/* DTE - Wait for DCE to be ready. */
		dtr_down(data);
		// MARKUS TODO: Configurable DTR activation timeout.
		set_dtr_timeout_event(data, DTR_TIMEOUT_DTR_ACTIVATION, K_MSEC(100));

		/* Continues in event_dce_ready_work. */

	} else {
		/* DCE - is ready at this point. */
		data->dtr_state = DTR_DOWN;
		activate_tx(data);
	}
}

static void event_up_work(struct dtr_uart_data *data)
{
	if (data->dtr_state == DTR_UP || data->dtr_state == DTR_DOWN_TO_UP) {
		LOG_WRN("DTR is already %s, ignoring event", "up");
		return;
	}

	data->dtr_state = DTR_DOWN_TO_UP;

	if (data->dte_role) {
		/* DTE */
		dtr_up(data);
	}

	deactivate_tx(data);
	deactivate_rx(data);

	/* Wait for UART to be disabled, before powering it down. */
}

static void event_rx_disabled_work(struct dtr_uart_data *data)
{
	if (data->dtr_state != DTR_DOWN_TO_UP) {
		LOG_INF("RX disabled event in state %s, ignoring event",
			dtr_state_str(data->dtr_state));
		return;
	}

	/* Power off UART module */
	const int err = power_off_uart(data);
	if (err) {
		LOG_ERR("Failed to suspend UART device: %d", err);
	}

	data->dtr_state = DTR_UP;
}

static void event_dce_ready_work(struct dtr_uart_data *data)
{
	/* DCE has notified DTE that it is ready, or sufficient time has passed. */
	data->dtr_state = DTR_DOWN;

	/* Send data that is ready to be transmitted. */
	activate_tx(data);

	notify_idle(data);
}

/**************************** WORKERS **********************************/
static void event_queue_work_fn(struct k_work *work)
{
	struct dtr_uart_data *data = CONTAINER_OF(work, struct dtr_uart_data, event_queue_work);
	enum dtr_uart_event evt;

	// if (data->user_callback == NULL || !data->app_rx_enable) {
	// 	LOG_INF("DTR events are not ready to be processed.");
	// 	return;
	// }

	while (k_msgq_get(&data->event_queue, &evt, K_NO_WAIT) == 0) {
		LOG_INF("DTR event \"%s\" in state %s", dtr_event_str(evt),
			dtr_state_str(data->dtr_state));

		switch (evt) {
		case DTR_UART_EVENT_UP:
		case DTR_UART_EVENT_IDLE:
			event_up_work(data);
			break;
		case DTR_UART_EVENT_RX_DISABLED:
			event_rx_disabled_work(data);
			break;
		case DTR_UART_EVENT_DOWN:
			event_down_work(data);
			break;
		case DTR_UART_EVENT_DCE_READY:
			event_dce_ready_work(data);
			break;
		default:
			LOG_WRN("Unknown event: %d", evt);
			break;
		}

		LOG_INF("DTR event \"%s\" done, new state %s", dtr_event_str(evt),
			dtr_state_str(data->dtr_state));
	}
}

static void dtr_timeout_work_fn(struct k_work *work)
{
	struct k_work_delayable *delayed_work = CONTAINER_OF(work, struct k_work_delayable, work);
	struct dtr_uart_data *data =
		CONTAINER_OF(delayed_work, struct dtr_uart_data, dtr_timeout_work);

	LOG_INF("DTR timeout event \"%s\" in state %s", dtr_timeout_str(data->dtr_timeout_event),
		dtr_state_str(data->dtr_state));

	switch (data->dtr_timeout_event) {
	case DTR_TIMEOUT_RI:
		nrf_gpio_pin_clear(data->ri_pin);
		break;
	case DTR_TIMEOUT_DTR_ACTIVATION:
		event_queue_delegate(data, DTR_UART_EVENT_DCE_READY);
		break;
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
	nrf_gpio_pin_set(data->ri_pin);
	set_dtr_timeout_event(data, DTR_TIMEOUT_RI, K_MSEC(1000));

}

/******************************* GPIO ********************************/

/* Handler is called when transition in state is detected which indicates
 * that for DCE, DTR is toggled or for DTE, RI is toggled.
 */
static void input_pin_handler(nrfx_gpiote_pin_t pin,
			     nrfx_gpiote_trigger_t trigger,
			     void *context)
{
	nrfx_gpiote_trigger_disable(get_gpiote(pin), pin);

	LOG_INF("Input pin handler called, pin: %d, trigger: %d", pin, trigger);

	struct dtr_uart_data *data = context;

	if (data->dte_role) {
		/* DTE - RI pin change from low to high triggers event. */
		if (data->dtr_state != DTR_DOWN && trigger == NRFX_GPIOTE_TRIGGER_HIGH) {
			event_queue_delegate(data, DTR_UART_EVENT_DOWN);

			/* MARKUS TODO: If we do not want to immediately bring the DTR DOWN with RI,
			 *              we would need to request a buffer, which the application
			 *              would take as a note that RI has risen. Buffer providing
			 *              or rx_enable would then be a sign to bring the DTR DOWN. */
		}
	} else {
		/* DCE - DTR triggered. */
		if (!data->dtr_pin_toggle) {
			/* DTR pin level change triggers a corresponding event. */
			if (trigger == NRFX_GPIOTE_TRIGGER_LOW) {
				event_queue_delegate(data, DTR_UART_EVENT_DOWN);
			} else {
				event_queue_delegate(data, DTR_UART_EVENT_UP);
			}
		} else if (trigger == NRFX_GPIOTE_TRIGGER_LOW) {
			/* DTR pin change from high to low triggers toggle. */
			if (data->dtr_state == DTR_UP || data->dtr_state == DTR_DOWN_TO_UP) {
				event_queue_delegate(data, DTR_UART_EVENT_DOWN);
			} else {
				event_queue_delegate(data, DTR_UART_EVENT_UP);
			}
		}
	}
	data->input_pin_trigger = trigger == NRFX_GPIOTE_TRIGGER_LOW ? NRFX_GPIOTE_TRIGGER_HIGH
								     : NRFX_GPIOTE_TRIGGER_LOW;
	k_work_schedule(&data->input_trigger_work, K_MSEC(data->dtr_pin_debounce));
}

static void input_trigger_work_fn(struct k_work *work)
{
	struct k_work_delayable *delayed_work = CONTAINER_OF(work, struct k_work_delayable, work);
	struct dtr_uart_data *data =
		CONTAINER_OF(delayed_work, struct dtr_uart_data, input_trigger_work);

	nrfx_gpiote_pin_t pin = data->dte_role ? data->ri_pin :	data->dtr_pin;
	int err;

	nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = data->input_pin_trigger,
	};

	nrf_gpio_pin_pull_t pull_config;

	if (data->dtr_pin_toggle) {
		/* Button must always have a pull-up resistor */
		pull_config = NRF_GPIO_PIN_PULLUP;
	} else if (trigger_config.trigger == NRFX_GPIOTE_TRIGGER_LOW) {
		pull_config = NRF_GPIO_PIN_PULLUP;
	} else {
		pull_config = NRF_GPIO_PIN_PULLDOWN;
	}

	nrfx_gpiote_handler_config_t handler_config = {
		.handler = input_pin_handler,
		.p_context = data
	};
	nrfx_gpiote_input_pin_config_t input_config = {
		.p_pull_config = &pull_config,
		.p_trigger_config = &trigger_config,
		.p_handler_config = &handler_config
	};

	err = nrfx_gpiote_input_configure(get_gpiote(pin), pin, &input_config);
	__ASSERT(err == NRFX_SUCCESS, "GPIO input configure (err:%d)", err);

	LOG_INF("input_trigger_work_fn, pin: %d, trigger: %d, pull: %d", pin,
		trigger_config.trigger, pull_config);

	nrfx_gpiote_trigger_enable(get_gpiote(pin), pin, true);
}

static int input_pin_init(struct dtr_uart_data *data, nrfx_gpiote_pin_t pin,
			  nrfx_gpiote_trigger_t trigger)
{
	__ASSERT(trigger == NRFX_GPIOTE_TRIGGER_LOW || trigger == NRFX_GPIOTE_TRIGGER_HIGH,
		 "Only level triggers are supported.");

	nrfx_err_t err;
	nrf_gpio_pin_pull_t pull_config =
		trigger == NRFX_GPIOTE_TRIGGER_HIGH ? NRF_GPIO_PIN_PULLDOWN : NRF_GPIO_PIN_PULLUP;

	nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = trigger,
	};

	nrfx_gpiote_handler_config_t handler_config = {
		.handler = input_pin_handler,
		.p_context = data
	};
	nrfx_gpiote_input_pin_config_t input_config = {
		.p_pull_config = &pull_config,
		.p_trigger_config = &trigger_config,
		.p_handler_config = &handler_config
	};

	LOG_INF("Input pin init, pin: %d, trigger: %d, pull: %d", pin, trigger, pull_config);

	nrfx_gpiote_trigger_disable(get_gpiote(pin), pin);

	err = nrfx_gpiote_input_configure(get_gpiote(pin), pin, &input_config);
	if (err != NRFX_SUCCESS) {
		LOG_INF("nrfx_gpiote_input_configure: %d", err);
		return -EINVAL;
	}

	LOG_INF("Input pin init, before enabling trigger");

	// nrfx_gpiote_trigger_enable(get_gpiote(pin), pin, true);

	LOG_INF("Input pin init done, pin: %d, trigger: %d, pull: %d", pin, trigger, pull_config);

	return 0;
}

static void init_input_pin_trigger(struct dtr_uart_data *data)
{
	static bool initialized;
	nrfx_gpiote_pin_t pin;

	if (!initialized) {
		pin = data->dte_role ? data->ri_pin : data->dtr_pin;
		nrfx_gpiote_trigger_enable(get_gpiote(pin), pin, true);
		initialized = true;
	}
}

static int output_pin_init(struct dtr_uart_data *data, nrfx_gpiote_pin_t pin, bool set)
{
	nrfx_err_t err;
	nrfx_gpiote_output_config_t output_config = NRFX_GPIOTE_DEFAULT_OUTPUT_CONFIG;

	err = nrfx_gpiote_output_configure(get_gpiote(pin), pin, &output_config, NULL);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("err:%08x", err);
		return -EINVAL;
	}

	if (set) {
		nrf_gpio_pin_set(pin);
	} else {
		nrf_gpio_pin_clear(pin);
	}

	return 0;
}

/****************************** UART ********************************/

static void user_callback(const struct device *dev, struct uart_event *evt)
{
	const struct dtr_uart_data *data = get_dev_data(dev);

	if (data->user_callback) {
		data->user_callback(dev, evt, data->user_data);
	}
}

static void uart_callback(const struct device *uart, struct uart_event *evt,
			  void *user_data)
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
		LOG_INF("RX: Ready buf:%p, offset: %d,len: %d",
		     (void *)evt->data.rx.buf, evt->data.rx.offset, evt->data.rx.len);
		user_callback(dev, evt);
		if (data->dtr_state == DTR_UP_TO_DOWN) {
			set_dtr_timeout_event(data, DTR_TIMEOUT_DTR_ACTIVATION, K_NO_WAIT);
		}
		notify_idle(data);
		break;

	case UART_RX_BUF_REQUEST:
		LOG_INF("UART_RX_BUF_REQUEST");
		user_callback(dev, evt);
		break;

	case UART_RX_BUF_RELEASED:
		LOG_INF("Rx buf released %p", (void *)evt->data.rx_buf.buf);
		user_callback(dev, evt);
		break;

	case UART_RX_DISABLED:
	{
		LOG_INF("UART_RX_DISABLED %d", data->dtr_state);
		user_callback(dev, evt);
		event_queue_delegate(data, DTR_UART_EVENT_RX_DISABLED);
		break;
	}
	case UART_RX_STOPPED:
		LOG_INF("Rx stopped");
		user_callback(dev, evt);
		break;
	}
}

static int dtr_uart_init(const struct device *dev)
{
	struct dtr_uart_data *data = get_dev_data(dev);
	const struct dtr_uart_config *cfg = get_dev_config(dev);
	int err;

	data->uart = DEVICE_DT_GET(DT_INST_BUS(0));
	if (!device_is_ready(data->uart)) {
		return -ENODEV;
	}
	data->dev = (const struct device *)dev;
	data->dte_role = cfg->dte_role;

	/* DTR */
	data->dtr_pin = cfg->dtr_pin;
	data->dtr_pin_toggle = cfg->dtr_pin_toggle;
	data->dtr_pin_debounce = cfg->dtr_pin_debounce;

	/* RI */
	data->ri_pin = cfg->ri_pin;

	k_msgq_init(&data->event_queue, (char *)&data->event_queue_buf, sizeof(enum dtr_uart_event),
		    sizeof(data->event_queue_buf) / sizeof(enum dtr_uart_event));
	k_work_init(&data->event_queue_work, event_queue_work_fn);

	k_work_init_delayable(&data->dtr_timeout_work, dtr_timeout_work_fn);
	k_work_init_delayable(&data->input_trigger_work, input_trigger_work_fn);

	if (data->dte_role) {
		/* DTE */
		/* At startup, set DTR high to disable DCE UART. */
		err = output_pin_init(data, data->dtr_pin, true);
		if (err < 0) {
			LOG_ERR("dtr pin init failed:%d", err);
			return err;
		}
		/* Expect high RI pulse from DCE. */
		err = input_pin_init(data, data->ri_pin, NRFX_GPIOTE_TRIGGER_HIGH);
		if (err < 0) {
			LOG_ERR("ri pin init failed:%d", err);
			return err;
		}
	} else {
		/* DCE */
		if (data->dtr_pin_toggle) {
			/* With DK, we start from (nearly) active.
			* Application calls uart_rx_enable to finalize UART activation.
			*/
			data->dtr_state = DTR_UP_TO_DOWN;  // MARKUS TODO: Unnecessary?
		}
		/* Expect DTR low from DTE to enable UART. */
		err = input_pin_init(data, data->dtr_pin, NRFX_GPIOTE_TRIGGER_LOW);
		if (err < 0) {
			LOG_ERR("dtr pin init failed:%d", err);
			return err;
		}

		/* Set sense to wake the DCE from possible shutdown */
		nrf_gpio_cfg_sense_set(data->dtr_pin, NRF_GPIO_PIN_SENSE_LOW);

		err = output_pin_init(data, data->ri_pin, false);
		if (err < 0) {
			LOG_ERR("ri pin init failed:%d", err);
			return err;
		}
	}
	err = uart_callback_set(data->uart, uart_callback, (void *)dev);
	if (err < 0) {
		return -EINVAL;
	}

	return err;
}

/**************************** API ********************************/

static int api_callback_set(const struct device *dev, uart_callback_t callback,
			    void *user_data)
{
	struct dtr_uart_data *data = get_dev_data(dev);

	data->user_callback = callback;
	data->user_data = user_data;

	return 0;
}

static int api_tx(const struct device *dev, const uint8_t *buf,
		  size_t len, int32_t timeout)
{
	if(buf == NULL || len == 0) {
		return 0;
	}
	LOG_INF("api_tx: %.*s", len, buf);

	struct dtr_uart_data *data = get_dev_data(dev);

	if (data->tx_buf) {
		LOG_ERR("TX scheduled");
		return -EBUSY;
	}

	notify_idle(data);

	if (data->dtr_state == DTR_DOWN) {
		return uart_tx(data->uart, buf, len, timeout);
	}
	data->tx_buf = buf;
	data->tx_len = len;

	if (data->dte_role) {
		/* DTE - Activate DCE and own UART. */
		event_queue_delegate(data, DTR_UART_EVENT_DOWN);
	} else {
		/* DCE - Start RI pulse. */
		ri_start(data);
	}

	/* Buffer the data until DTR is down. */
	return 0;
}

static int api_tx_abort(const struct device *dev)
{
	return -ENOTSUP;
}

/* Application calls this when it is ready to receive data. */
static int api_rx_enable(const struct device *dev, uint8_t *buf,
			 size_t len, int32_t timeout)
{
	LOG_INF("api_rx_enable: %p, %zu", (void *)buf, len);

	struct dtr_uart_data *data = get_dev_data(dev);

	// MARKUS TODO: Multiple calls to here must not overwrite existing buffers.
	// MARKUS TODO: We must release all the buffers.
	data->rx_buf = buf;
	data->rx_len = len;

	init_input_pin_trigger(data);

	if (!data->app_rx_enable) {
		data->app_rx_enable = true;
		if (!data->dte_role) {
			/* DCE */
			if (data->dtr_pin_toggle) {
				/* With toggle, we start from DOWN state. */
				event_queue_delegate(data, DTR_UART_EVENT_DOWN);
			} else if (nrf_gpio_pin_read(data->dtr_pin)) {
				/* Suspend UART in startup, if DTR signal is UP. */
				LOG_INF("Suspend UART in startup.");
				power_off_uart(data);
				data->dtr_state = DTR_UP;
			}
		}
	}

	if (data->dte_role) {
		/* DTE */
		if (IS_ENABLED(CONFIG_NRF_SW_DTR_UART_DTE_IDLE)) {
			/* If we use idle, UART is activated with TX operation or with RI pulse. */
			LOG_INF("Suspend UART in startup.");
			power_off_uart(data);
			data->dtr_state = DTR_UP;
		} else {
			/* Activate DTR. */
			event_queue_delegate(data, DTR_UART_EVENT_DOWN);
		}
	}

	return 0;
}

static int api_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct dtr_uart_data *data = get_dev_data(dev);

	if (data->rx_buf_requested) {
		LOG_INF("Buf requested for RX start.");
		data->rx_buf = buf;
		data->rx_len = len;
		return 0;
	}

	LOG_INF("api_rx_buf_rsp: %p, %zu", (void *)buf, len);

	return uart_rx_buf_rsp(data->uart, buf, len);
}

static int api_rx_disable(const struct device *dev)
{
	struct dtr_uart_data *data = get_dev_data(dev);

	LOG_INF("api_rx_disable");

	if (data->dte_role) {
		if (!IS_ENABLED(CONFIG_NRF_SW_DTR_UART_DTE_IDLE)) {
			event_queue_delegate(data, DTR_UART_EVENT_UP);
		}
		return 0;
	}

	/* DCE cannot disable RX. */ // MARKUS TODO: Unsure what this should do in the application.
	return -ENOTSUP;
}

static int api_err_check(const struct device *dev)
{
	struct dtr_uart_data *data = get_dev_data(dev);

	return uart_err_check(data->uart);
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int api_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct dtr_uart_data *data = get_dev_data(dev);

	// if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
	// 	return -ENOTSUP;
	// }

	return uart_configure(data->uart, cfg);
}

static int api_config_get(const struct device *dev, struct uart_config *cfg)
{
	const struct dtr_uart_data *data = get_dev_data(dev);

	return uart_config_get(data->uart, cfg);
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static const struct dtr_uart_config dtr_uart_config = {
	.dte_role = DT_INST_PROP(0, dte_role),

	.dtr_pin = DT_INST_PROP(0, dtr_pin),  // MARKUS TODO: Configure in the device tree.
	.dtr_pin_toggle = DT_INST_PROP(0, dtr_pin_toggle),
	.dtr_pin_debounce = DT_INST_PROP(0, dtr_pin_debounce),

	.ri_pin = DT_INST_PROP(0, ri_pin),
};

static struct dtr_uart_data dtr_uart_data;

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

#define GPIO_HAS_PIN(gpio_node, pin_prop)				  \
	(DT_PROP(gpio_node, port) == (DT_INST_PROP(0, pin_prop) >> 5))

/* There may be GPIO ports which cannot be used with GPIOTE. Check if pins are
 * not from those ports.
 */
#define CHECK_GPIOTE_AVAILABLE(gpio_node) \
	BUILD_ASSERT((!GPIO_HAS_PIN(gpio_node, dtr_pin) &&		  \
		      !GPIO_HAS_PIN(gpio_node, ri_pin)) ||		  \
		     DT_NODE_HAS_PROP(gpio_node, gpiote_instance));

DT_FOREACH_STATUS_OKAY(nordic_nrf_gpio, CHECK_GPIOTE_AVAILABLE)

DEVICE_DT_DEFINE(DT_NODELABEL(dtr_uart), dtr_uart_init, NULL,
	      &dtr_uart_data, &dtr_uart_config,
	      POST_KERNEL, CONFIG_NRF_SW_DTR_UART_INIT_PRIORITY,
	      &dtr_uart_api);
