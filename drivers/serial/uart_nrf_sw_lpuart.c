/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
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
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

LOG_MODULE_REGISTER(lpuart, CONFIG_NRF_SW_LPUART_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_sw_lpuart

/* States. */
enum rx_state {
	/* RX is disabled. */
	RX_OFF,

	/* RX is in low power, idle state with pin detection armed. */
	RX_IDLE,

	/* RX request is pending, receiver is in preparation. */
	RX_PREPARE,

	/* RX is in active state, receiver is running. */
	RX_ACTIVE,

	/* RX is transitioning to from active idle state. */
	RX_TO_IDLE,

	/* RX is waiting for a new buffer. */
	RX_BLOCKED,

	/* RX is transitioning to off state. */
	RX_TO_OFF,
};

#if CONFIG_NRF_SW_LPUART_INT_DRIVEN
struct lpuart_int_driven {
	uart_irq_callback_user_data_t callback;
	void *user_data;

	uint8_t txbuf[CONFIG_NRF_SW_LPUART_INT_DRV_TX_BUF_SIZE];
	size_t txlen;

	uint8_t rxbuf[CONFIG_NRF_SW_LPUART_MAX_PACKET_SIZE];
	size_t rxlen;
	size_t rxrd;

	bool tx_enabled;
	bool rx_enabled;
	bool err_enabled;
};
#endif

/* Low power uart structure. */
struct lpuart_data {
	/* Physical UART device */
	const struct device *uart;

	/* Request pin. */
	nrfx_gpiote_pin_t req_pin;
	/* Response pin. */
	nrfx_gpiote_pin_t rdy_pin;

	/* GPIOTE channel used by rdy pin. */
	uint8_t rdy_ch;

	/* Timer used for TX timeouting. */
	struct k_timer tx_timer;

	/* Buffer used for poll out. */
	atomic_t txbyte;

	/* Current TX buffer. */
	const uint8_t *tx_buf;
	size_t tx_len;

	/* Set to true if physical transfer is started. */
	bool tx_active;

	/* Set to true if any data was received. */
	bool rx_got_data;

	/* Current RX buffer. */
	uint8_t *rx_buf;
	size_t rx_len;

	int32_t rx_timeout;

	uart_callback_t user_callback;
	void *user_data;

	/* RX state */
	enum rx_state rx_state;

	struct onoff_client rx_clk_cli;

#if CONFIG_NRF_SW_LPUART_INT_DRIVEN
	struct lpuart_int_driven int_driven;
#endif
};

/* Configuration structured. */
struct lpuart_config {
	nrfx_gpiote_pin_t req_pin;
	nrfx_gpiote_pin_t rdy_pin;
};

static void req_pin_handler(nrfx_gpiote_pin_t pin,
			     nrfx_gpiote_trigger_t trigger,
			     void *context);

static void rdy_pin_handler(nrfx_gpiote_pin_t pin,
			     nrfx_gpiote_trigger_t trigger,
			     void *context);

static inline struct lpuart_data *get_dev_data(const struct device *dev)
{
	return dev->data;
}

static inline const struct lpuart_config *get_dev_config(const struct device *dev)
{
	return dev->config;
}

#define GPIOTE_NODE(gpio_node) DT_PHANDLE(gpio_node, gpiote_instance)
#define GPIOTE_INST_AND_COMMA(gpio_node) \
	[DT_PROP(gpio_node, port)] = \
		NRFX_GPIOTE_INSTANCE(DT_PROP(GPIOTE_NODE(gpio_node), instance)),

static const nrfx_gpiote_t *get_gpiote(nrfx_gpiote_pin_t pin)
{
	static const nrfx_gpiote_t gpiote[GPIO_COUNT] = {
		DT_FOREACH_STATUS_OKAY(nordic_nrf_gpio, GPIOTE_INST_AND_COMMA)
	};

	return &gpiote[pin >> 5];
}

/* Called when uart transfer is finished to indicate to the receiver that it
 * can be closed.
 */
static void req_pin_idle(struct lpuart_data *data)
{
	nrf_gpio_cfg(data->req_pin,
		     NRF_GPIO_PIN_DIR_OUTPUT,
		     NRF_GPIO_PIN_INPUT_DISCONNECT,
		     NRF_GPIO_PIN_NOPULL,
		     NRF_GPIO_PIN_S0S1,
		     NRF_GPIO_PIN_NOSENSE);
}

static void pend_req_pin_idle(struct lpuart_data *data)
{
	/* Wait until pin is high */
	while (!nrfx_gpiote_in_is_set(data->req_pin)) {
	}
}

/* Used to force pin assertion. Pin is kept high during uart transfer. */
static void req_pin_set(struct lpuart_data *data)
{
	const nrf_gpio_pin_dir_t dir = NRF_GPIO_PIN_DIR_INPUT;
	const nrf_gpio_pin_input_t input = NRF_GPIO_PIN_INPUT_CONNECT;

	nrf_gpio_reconfigure(data->req_pin, &dir, &input, NULL, NULL, NULL);

	nrfx_gpiote_trigger_disable(get_gpiote(data->req_pin), data->req_pin);
}

/* Pin is reconfigured to input with pull up and low state detection. That leads
 * to pin assertion. Receiver when ready will pull pin down for a moment which
 * means that transmitter can start.
 */
static void req_pin_arm(struct lpuart_data *data)
{
	const nrf_gpio_pin_pull_t pull = NRF_GPIO_PIN_PULLUP;

	/* Add pull up before reconfiguring to input. */
	nrf_gpio_reconfigure(data->req_pin, NULL, NULL, &pull, NULL, NULL);

	nrfx_gpiote_trigger_enable(get_gpiote(data->req_pin), data->req_pin, true);
}

static int req_pin_init(struct lpuart_data *data, nrfx_gpiote_pin_t pin)
{
	uint8_t ch;
	nrfx_err_t err;
	nrf_gpio_pin_pull_t pull_config = NRF_GPIO_PIN_PULLDOWN;
	nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_HITOLO,
		.p_in_channel = &ch
	};
	nrfx_gpiote_handler_config_t handler_config = {
		.handler = req_pin_handler,
		.p_context = data
	};
	nrfx_gpiote_input_pin_config_t input_config = {
		.p_pull_config = &pull_config,
		.p_trigger_config = &trigger_config,
		.p_handler_config = &handler_config
	};

	err = nrfx_gpiote_channel_alloc(get_gpiote(pin), &ch);
	if (err != NRFX_SUCCESS) {
		return -ENOMEM;
	}

	err = nrfx_gpiote_input_configure(get_gpiote(pin), pin, &input_config);
	if (err != NRFX_SUCCESS) {
		return -EINVAL;
	}

	data->req_pin = pin;
	/* Make gpiote driver think that pin is input but then reconfigure to
	 * output. Which will be at some point reconfigured back to input for
	 * a moment.
	 */
	req_pin_idle(data);

	return 0;
}

static void rdy_pin_suspend(struct lpuart_data *data)
{
	nrfx_gpiote_trigger_disable(get_gpiote(data->rdy_pin), data->rdy_pin);
}


static int rdy_pin_init(struct lpuart_data *data, nrfx_gpiote_pin_t pin)
{
	nrfx_err_t err;
	nrf_gpio_pin_pull_t pull_config = NRF_GPIO_PIN_NOPULL;
	nrfx_gpiote_handler_config_t handler_config = {
		.handler = rdy_pin_handler,
		.p_context = data
	};
	nrfx_gpiote_input_pin_config_t input_config = {
		.p_pull_config = &pull_config,
		.p_trigger_config = NULL,
		.p_handler_config = &handler_config
	};

	err = nrfx_gpiote_channel_alloc(get_gpiote(pin), &data->rdy_ch);
	if (err != NRFX_SUCCESS) {
		return -ENOMEM;
	}

	err = nrfx_gpiote_input_configure(get_gpiote(pin), pin, &input_config);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("err:%08x", err);
		return -EINVAL;
	}

	data->rdy_pin = pin;
	nrf_gpio_pin_clear(pin);

	return 0;
}

/* Pin activated to detect high state (using SENSE). */
static void rdy_pin_idle(struct lpuart_data *data)
{
	nrfx_err_t err;
	nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_HIGH
	};
	nrfx_gpiote_input_pin_config_t input_config = {
		.p_pull_config = NULL,
		.p_trigger_config = &trigger_config,
		.p_handler_config = NULL
	};
	const nrfx_gpiote_t *gpiote = get_gpiote(data->rdy_pin);

	err = nrfx_gpiote_input_configure(gpiote, data->rdy_pin, &input_config);
	__ASSERT(err == NRFX_SUCCESS, "Unexpected err: %08x/%d", err, err);

	nrfx_gpiote_trigger_enable(gpiote, data->rdy_pin, true);
}

/* Indicated to the transmitter that receiver is ready by pulling pin down for
 * a moment, and reconfiguring it back to input with low state detection.
 *
 * Function checks if transmitter has request pin in the expected state and if not
 * false is returned.
 */
static bool rdy_pin_blink(struct lpuart_data *data)
{
	nrfx_err_t err;
	nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_HITOLO,
		.p_in_channel = &data->rdy_ch
	};
	nrfx_gpiote_input_pin_config_t input_config = {
		.p_pull_config = NULL,
		.p_trigger_config = &trigger_config,
		.p_handler_config = NULL
	};
	const nrf_gpio_pin_dir_t dir_in = NRF_GPIO_PIN_DIR_INPUT;
	const nrf_gpio_pin_dir_t dir_out = NRF_GPIO_PIN_DIR_OUTPUT;
	const nrfx_gpiote_t *gpiote = get_gpiote(data->rdy_pin);
	bool ret;

	/* Drive low for a moment */
	nrf_gpio_reconfigure(data->rdy_pin, &dir_out, NULL, NULL, NULL, NULL);

	err = nrfx_gpiote_input_configure(gpiote, data->rdy_pin, &input_config);
	__ASSERT(err == NRFX_SUCCESS, "Unexpected err: %08x/%d", err, err);

	nrfx_gpiote_trigger_enable(gpiote, data->rdy_pin, true);

	int key = irq_lock();

	nrf_gpiote_event_t event = nrf_gpiote_in_event_get(data->rdy_ch);

	nrf_gpio_reconfigure(data->rdy_pin, &dir_in, NULL, NULL, NULL, NULL);

	/* Wait a bit, after switching to input transmitter pin pullup should drive
	 * this pin high.
	 */
	k_busy_wait(1);
	if (nrf_gpio_pin_read(data->rdy_pin) == 0 && !nrf_gpiote_event_check(NRF_GPIOTE, event)) {
		/* Suspicious pin state (low). It might be that context was preempted
		 * for long enough and transfer ended (in that case event will be set)
		 * or transmitter is working abnormally or pin is just floating.
		 */
		ret = false;
		LOG_WRN("req pin low when expected high");
	} else {
		ret = true;
	}
	irq_unlock(key);

	return ret;
}

/* Called when end of transfer is detected. It sets response pin to idle and
 * disables RX.
 */
static void deactivate_rx(struct lpuart_data *data)
{
	int err;

	if (IS_ENABLED(CONFIG_NRF_SW_LPUART_HFXO_ON_RX)) {
		struct onoff_manager *mgr =
		     z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);

		err = onoff_cancel_or_release(mgr, &data->rx_clk_cli);
		__ASSERT_NO_MSG(err >= 0);
	}

	/* abort rx */
	data->rx_state = RX_TO_IDLE;
	err = uart_rx_disable(data->uart);
	if (err < 0 && err != -EFAULT) {
		LOG_ERR("RX: Failed to disable (err: %d)", err);
	} else if (err == -EFAULT) {
		LOG_ERR("Rx disable failed.");
	}
}

/* Function enables RX, after that it informs transmitter about readiness by
 * clearing the pin (out, low) and then reconfigures to in, pullup + high to low
 * detection to detect end of transfer.
 */
static void activate_rx(struct lpuart_data *data)
{
	int err;

	if (data->rx_buf == NULL) {
		LOG_ERR("RX: Request before enabling RX");
		return;
	}

	LOG_DBG("RX: Ready");
	data->rx_got_data = false;
	data->rx_state = RX_ACTIVE;
	err = uart_rx_enable(data->uart, data->rx_buf,
				data->rx_len, data->rx_timeout);
	__ASSERT(err == 0, "RX: Enabling failed (err:%d)", err);

	/* Ready. Confirm by toggling the pin. */
	if (!rdy_pin_blink(data)) {
		/* If tranmitter behaves abnormally deactivate RX. */
		rdy_pin_suspend(data);
		deactivate_rx(data);
	}
}

static void rx_hfclk_callback(struct onoff_manager *mgr,
			      struct onoff_client *cli,
			      uint32_t state, int res)
{
	struct lpuart_data *data =
		CONTAINER_OF(cli, struct lpuart_data, rx_clk_cli);

	__ASSERT_NO_MSG(res >= 0);

	activate_rx(data);
}

static void rx_hfclk_request(struct lpuart_data *data)
{
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	int err;

	sys_notify_init_callback(&data->rx_clk_cli.notify, rx_hfclk_callback);
	err = onoff_request(mgr, &data->rx_clk_cli);
	__ASSERT_NO_MSG(err >= 0);
}

static void start_rx_activation(struct lpuart_data *data)
{
	data->rx_state = RX_PREPARE;

	if (IS_ENABLED(CONFIG_NRF_SW_LPUART_HFXO_ON_RX)) {
		rx_hfclk_request(data);
	} else {
		activate_rx(data);
	}
}

static void tx_complete(struct lpuart_data *data)
{
	LOG_DBG("TX completed, pin idle");
	if (data->tx_active) {
		pend_req_pin_idle(data);
	} else {
		req_pin_set(data);
	}

	req_pin_idle(data);
	data->tx_buf = NULL;
	data->tx_active = false;
}

/* Handler is called when transition to low state is detected which indicates
 * that receiver is ready for the packet.
 */
static void req_pin_handler(nrfx_gpiote_pin_t pin,
			     nrfx_gpiote_trigger_t trigger,
			     void *context)
{
	ARG_UNUSED(trigger);
	ARG_UNUSED(pin);
	int key;
	const uint8_t *buf;
	size_t len;
	int err;
	struct lpuart_data *data = context;

	if (data->tx_buf == NULL) {
		LOG_WRN("TX: request confirmed but no data to send");
		tx_complete(data);
		/* aborted */
		return;
	}

	LOG_DBG("TX: Confirmed, starting.");

	req_pin_set(data);
	k_timer_stop(&data->tx_timer);

	key = irq_lock();
	data->tx_active = true;
	buf = data->tx_buf;
	len = data->tx_len;
	irq_unlock(key);
	err = uart_tx(data->uart, buf, len, 0);
	if (err < 0) {
		LOG_ERR("TX: Not started (error: %d)", err);
		tx_complete(data);
	}
}

/* Handler is called in two cases:
 * - high state detection. Receiver is idle and new transfer request is received
 * - low state. Receiver is active and receiving a packet. Transmitter indicates
 *   end of the packet.
 */
static void rdy_pin_handler(nrfx_gpiote_pin_t pin,
			    nrfx_gpiote_trigger_t trigger,
			    void *context)
{
	struct lpuart_data *data = context;

	rdy_pin_suspend(data);
	if (trigger == NRFX_GPIOTE_TRIGGER_HIGH) {
		__ASSERT_NO_MSG(data->rx_state != RX_ACTIVE);

		LOG_DBG("RX: Request detected.");
		if (data->rx_state == RX_IDLE) {
			start_rx_activation(data);
		}
	} else { /* HITOLO */
		if (data->rx_state != RX_ACTIVE) {
			LOG_WRN("RX: End detected at unexpected state (%d).", data->rx_state);
			data->rx_state = RX_IDLE;
			rdy_pin_idle(data);
			return;
		}

		LOG_DBG("RX: End detected.");
		deactivate_rx(data);
	}
}

static int api_callback_set(const struct device *dev, uart_callback_t callback,
			    void *user_data)
{
	struct lpuart_data *data = get_dev_data(dev);

	data->user_callback = callback;
	data->user_data = user_data;

	return 0;
}

static void user_callback(const struct device *dev, struct uart_event *evt)
{
	const struct lpuart_data *data = get_dev_data(dev);

	if (data->user_callback) {
		data->user_callback(dev, evt, data->user_data);
	}
}

static void uart_callback(const struct device *uart, struct uart_event *evt,
			  void *user_data)
{
	struct device *dev = user_data;
	struct lpuart_data *data = get_dev_data(dev);

	switch (evt->type) {
	case UART_TX_DONE:
	{
		const uint8_t *txbuf = evt->data.tx.buf;

		tx_complete(data);
		if (txbuf == (void *)&data->txbyte) {
			data->txbyte = -1;
		} else {
			user_callback(dev, evt);
		}

		break;
	}
	case UART_TX_ABORTED:
		LOG_DBG("tx aborted");
		user_callback(dev, evt);
		break;

	case UART_RX_RDY:
		LOG_DBG("RX: Ready buf:%p, offset: %d,len: %d",
		     (void *)evt->data.rx.buf, evt->data.rx.offset, evt->data.rx.len);
		data->rx_got_data = true;
		user_callback(dev, evt);
		break;

	case UART_RX_BUF_REQUEST:
		/* If packet will fit in the provided buffer do not request
		 * additional buffer.
		 */
		if (data->rx_len < CONFIG_NRF_SW_LPUART_MAX_PACKET_SIZE) {
			user_callback(dev, evt);
		}
		break;

	case UART_RX_BUF_RELEASED:
		LOG_DBG("Rx buf released");
		if (!data->rx_got_data) {
			LOG_ERR("Empty receiver state:%d", data->rx_state);
		}
		user_callback(dev, evt);
		break;

	case UART_RX_DISABLED:
	{
		bool call_cb;

		LOG_DBG("Rx disabled");
		__ASSERT_NO_MSG((data->rx_state != RX_IDLE) &&
			 (data->rx_state != RX_OFF));

		if (data->rx_state == RX_TO_IDLE) {
			if (data->rx_got_data) {
				call_cb = true;
				data->rx_state = RX_BLOCKED;
				/* Need to request new buffer since uart was disabled */
				evt->type = UART_RX_BUF_REQUEST;
			} else {
				call_cb = false;
				data->rx_state = RX_IDLE;
				rdy_pin_idle(data);
			}
		} else {
			data->rx_buf = NULL;
			data->rx_state = RX_OFF;
			call_cb = true;
		}

		if (call_cb) {
			user_callback(dev, evt);
		}
		break;
	}
	case UART_RX_STOPPED:
		LOG_DBG("Rx stopped");
		user_callback(dev, evt);
		break;
	}
}

static void tx_timeout(struct k_timer *timer)
{
	struct device *dev = k_timer_user_data_get(timer);
	struct lpuart_data *data = get_dev_data(dev);
	const uint8_t *txbuf = data->tx_buf;
	int err;

	LOG_WRN("Tx timeout");
	if (data->tx_active) {
		err = uart_tx_abort(data->uart);
		if (err == -EFAULT) {
			LOG_DBG("No active transfer. Already finished?");
		} else if (err < 0) {
			__ASSERT(0, "Unexpected tx_abort error:%d", err);
		}
		return;
	}

	tx_complete(data);

	if (txbuf == (void *)&data->txbyte) {
		data->txbyte = -1;
	} else {
		struct uart_event evt = {
			.type = UART_TX_ABORTED,
			.data = {
				.tx = {
					.buf = txbuf,
					.len = 0
				}
			}
		};

		user_callback(dev, &evt);
	}
}

static int api_tx(const struct device *dev, const uint8_t *buf,
		  size_t len, int32_t timeout)
{
	struct lpuart_data *data = get_dev_data(dev);

	if (!atomic_ptr_cas((atomic_ptr_t *)&data->tx_buf, NULL, (void *)buf)) {
		return -EBUSY;
	}

	LOG_DBG("tx len:%d", len);
	data->tx_len = len;
	k_timer_start(&data->tx_timer,
		      timeout == SYS_FOREVER_US ? K_FOREVER : K_USEC(timeout),
		      K_NO_WAIT);

	/* Enable interrupt on pin going low. */
	req_pin_arm(data);

	return 0;
}

static int api_tx_abort(const struct device *dev)
{
	struct lpuart_data *data = get_dev_data(dev);
	const uint8_t *buf = data->tx_buf;
	int err;
	int key;

	if (data->tx_buf == NULL) {
		return -EFAULT;
	}

	k_timer_stop(&data->tx_timer);
	key = irq_lock();
	tx_complete(data);
	irq_unlock(key);

	err = uart_tx_abort(data->uart);
	if (err != -EFAULT) {
		/* if successfully aborted or returned error different than
		 * one indicating that there is no transfer, return error code.
		 */
		return err;
	}

	struct uart_event event = {
		.type = UART_TX_ABORTED,
		.data = {
			.tx = {
				.buf = buf,
				.len = 0
			}
		}
	};

	user_callback(dev, &event);

	return err;
}

static int api_rx_enable(const struct device *dev, uint8_t *buf,
			 size_t len, int32_t timeout)
{
	struct lpuart_data *data = get_dev_data(dev);

	__ASSERT_NO_MSG(data->rx_state == RX_OFF);

	if (!atomic_ptr_cas((atomic_ptr_t *)&data->rx_buf, NULL, buf)) {
		return -EBUSY;
	}

	data->rx_len = len;
	data->rx_timeout = timeout;
	data->rx_state = RX_IDLE;

	LOG_DBG("RX: Enabling");
	rdy_pin_idle(data);

	return 0;
}

static int api_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct lpuart_data *data = get_dev_data(dev);

	__ASSERT_NO_MSG((data->rx_state != RX_OFF) &&
		 (data->rx_state != RX_TO_OFF));

	LOG_DBG("buf rsp, state:%d", data->rx_state);
	if (data->rx_state == RX_TO_IDLE || data->rx_state == RX_BLOCKED) {
		data->rx_buf = buf;
		data->rx_len = len;

		data->rx_state = RX_IDLE;
		rdy_pin_idle(data);

		return 0;
	}

	return uart_rx_buf_rsp(data->uart, buf, len);
}

static int api_rx_disable(const struct device *dev)
{
	struct lpuart_data *data = get_dev_data(dev);
	int err;

	if (data->rx_state == RX_OFF) {
		return -EFAULT;
	}

	data->rx_state = RX_TO_OFF;

	err = uart_rx_disable(data->uart);
	if (err == -EFAULT) {
		struct uart_event event = {
			.type = UART_RX_DISABLED
		};

		data->rx_state = RX_OFF;
		data->rx_buf = NULL;
		user_callback(dev, &event);
	}

	return 0;
}

#if CONFIG_NRF_SW_LPUART_INT_DRIVEN

static uint32_t int_driven_rd_available(struct lpuart_data *data)
{
	return data->int_driven.rxlen - data->int_driven.rxrd;
}

static void int_driven_rx_feed(const struct device *dev,
			       struct lpuart_data *data)
{
	int err;

	data->int_driven.rxlen = 0;
	data->int_driven.rxrd = 0;
	err = api_rx_buf_rsp(dev, data->int_driven.rxbuf,
			     sizeof(data->int_driven.rxbuf));
	__ASSERT_NO_MSG(err >= 0);
}

static void int_driven_evt_handler(const struct device *lpuart,
				   struct uart_event *evt,
				   void *user_data)
{
	struct lpuart_data *data = get_dev_data(lpuart);
	bool call_handler = false;

	switch (evt->type) {
	case UART_TX_DONE:
		data->int_driven.txlen = 0;
		call_handler = true;
		break;
	case UART_RX_RDY:
		__ASSERT_NO_MSG(data->int_driven.rxlen == 0);
		data->int_driven.rxlen = evt->data.rx.len;
		call_handler = data->int_driven.rx_enabled;
		break;
	case UART_RX_BUF_REQUEST:
		if (int_driven_rd_available(data) == 0) {
			int_driven_rx_feed(lpuart, data);
		}
		break;
	case UART_RX_STOPPED:
		call_handler = data->int_driven.err_enabled;
		break;
	case UART_RX_DISABLED:
	{
		int err;

		data->int_driven.rxlen = 0;
		data->int_driven.rxrd = 0;
		err = api_rx_enable(lpuart, data->int_driven.rxbuf,
					sizeof(data->int_driven.rxbuf), 1000);
		__ASSERT_NO_MSG(err >= 0);
		break;
	}
	default:
		break;
	}

	if (call_handler) {
		data->int_driven.callback(lpuart, data->int_driven.user_data);
	}
}

static int api_fifo_read(const struct device *dev,
			 uint8_t *rx_data,
			 const int size)
{
	struct lpuart_data *data = get_dev_data(dev);
	uint32_t available = int_driven_rd_available(data);
	uint32_t cpylen = 0;

	if (available) {
		cpylen = MIN(available, size);
		memcpy(rx_data,
		       &data->int_driven.rxbuf[data->int_driven.rxrd],
		       cpylen);
		data->int_driven.rxrd += cpylen;
	}

	return cpylen;
}

static int api_fifo_fill(const struct device *dev,
			 const uint8_t *tx_data,
			 int size)
{
	struct lpuart_data *data = get_dev_data(dev);
	int err;

	size = MIN(size, sizeof(data->int_driven.txbuf));
	if (!atomic_cas((atomic_t *)&data->int_driven.txlen, 0, size)) {
		return 0;
	}

	memcpy(data->int_driven.txbuf, tx_data, size);

	err = api_tx(dev, data->int_driven.txbuf,
		     data->int_driven.txlen,
		     CONFIG_NRF_SW_LPUART_DEFAULT_TX_TIMEOUT);
	if (err < 0) {
		data->int_driven.txlen = 0;

		return 0;
	}

	return size;
}

static void api_irq_tx_enable(const struct device *dev)
{
	struct lpuart_data *data = get_dev_data(dev);

	data->int_driven.tx_enabled = true;
	if (data->tx_buf == NULL) {
		data->int_driven.callback(dev, data->int_driven.user_data);
	}
}

static void api_irq_tx_disable(const struct device *dev)
{
	struct lpuart_data *data = get_dev_data(dev);

	data->int_driven.tx_enabled = false;
}

static int api_irq_tx_ready(const struct device *dev)
{
	struct lpuart_data *data = get_dev_data(dev);

	return data->int_driven.tx_enabled && (data->tx_buf == NULL);
}

static void api_irq_callback_set(const struct device *dev,
				 uart_irq_callback_user_data_t cb,
				 void *user_data)
{
	struct lpuart_data *data = get_dev_data(dev);

	data->int_driven.callback = cb;
	data->int_driven.user_data = user_data;
}

static void api_irq_rx_enable(const struct device *dev)
{
	struct lpuart_data *data = get_dev_data(dev);

	data->int_driven.rx_enabled = true;
	if (int_driven_rd_available(data)) {
		data->int_driven.callback(dev, data->int_driven.user_data);
	}

	/* If RX was disabled and there were pending data, it may have been
	 * processed above after re-enabling. If whole data has been processed
	 * we must feed the buffer back to the uart to allow reception of the
	 * next packet.
	 */
	if (!int_driven_rd_available(data) && data->rx_state == RX_BLOCKED) {
		/* Whole packet read, RX can be re-enabled. */
		int_driven_rx_feed(dev, data);
	}
}

static void api_irq_rx_disable(const struct device *dev)
{
	struct lpuart_data *data = get_dev_data(dev);

	data->int_driven.rx_enabled = false;
}

static int api_irq_rx_ready(const struct device *dev)
{
	struct lpuart_data *data = get_dev_data(dev);

	return data->int_driven.rx_enabled &&
		int_driven_rd_available(get_dev_data(dev));
}

static int api_irq_tx_complete(const struct device *dev)
{
	return api_irq_tx_ready(dev);
}

static void api_irq_err_enable(const struct device *dev)
{
	struct lpuart_data *data = get_dev_data(dev);

	data->int_driven.err_enabled = true;
}

static void api_irq_err_disable(const struct device *dev)
{
	struct lpuart_data *data = get_dev_data(dev);

	data->int_driven.err_enabled = false;
}

static int api_irq_is_pending(const struct device *dev)
{
	return api_irq_rx_ready(dev) || api_irq_tx_ready(dev);
}

static int api_irq_update(const struct device *dev)
{
	return 1;
}

#endif /* CONFIG_NRF_SW_LPUART_INT_DRIVEN */

static int lpuart_init(const struct device *dev)
{
	struct lpuart_data *data = get_dev_data(dev);
	const struct lpuart_config *cfg = get_dev_config(dev);
	int err;

	data->uart = DEVICE_DT_GET(DT_INST_BUS(0));
	if (!device_is_ready(data->uart)) {
		return -ENODEV;
	}

	err = req_pin_init(data, cfg->req_pin);
	if (err < 0) {
		LOG_ERR("req pin init failed:%d", err);
		return err;
	}

	err = rdy_pin_init(data, cfg->rdy_pin);
	if (err < 0) {
		LOG_ERR("rdy pin init failed:%d", err);
		return err;
	}

	k_timer_init(&data->tx_timer, tx_timeout, NULL);
	k_timer_user_data_set(&data->tx_timer, (void *)dev);

	err = uart_callback_set(data->uart, uart_callback, (void *)dev);
	if (err < 0) {
		return -EINVAL;
	}

#if CONFIG_NRF_SW_LPUART_INT_DRIVEN
	err = uart_callback_set(dev, int_driven_evt_handler, NULL);
	if (err < 0) {
		return -EINVAL;
	}

	err = api_rx_enable(dev, data->int_driven.rxbuf,
				sizeof(data->int_driven.rxbuf), 1000);
#endif

	data->txbyte = -1;

	return err;
}

static int api_poll_in(const struct device *dev, unsigned char *p_char)
{
#if CONFIG_NRF_SW_LPUART_INT_DRIVEN
	return api_fifo_read(dev, p_char, 1) ? 0 : -1;
#else
	return -ENOTSUP;
#endif
}

static void api_poll_out(const struct device *dev, unsigned char out_char)
{
	struct lpuart_data *data = get_dev_data(dev);
	bool thread_ctx = !k_is_in_isr() && !k_is_pre_kernel();
	int err;

	if (thread_ctx) {
		/* in thread context pend until tx is in idle */
		while (data->tx_buf) {
			k_msleep(1);
		}
	} else if (data->tx_buf) {
		return;
	}

	if (!atomic_cas(&data->txbyte, -1, out_char)) {
		return;
	}

	err = api_tx(dev, (uint8_t *)&data->txbyte, 1,
		     CONFIG_NRF_SW_LPUART_DEFAULT_TX_TIMEOUT);
	if (err < 0) {
		data->txbyte = -1;
	}
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int api_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct lpuart_data *data = get_dev_data(dev);

	if (cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOTSUP;
	}

	return uart_configure(data->uart, cfg);
}

static int api_config_get(const struct device *dev, struct uart_config *cfg)
{
	const struct lpuart_data *data = get_dev_data(dev);

	return uart_config_get(data->uart, cfg);
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#define LPUART_PIN_CFG_INITIALIZER(pin_prop) \
	{ \
		.pin = DT_INST_PROP(0, pin_prop) \
	}

static const struct lpuart_config lpuart_config = {
	.req_pin = DT_INST_PROP(0, req_pin),
	.rdy_pin = DT_INST_PROP(0, rdy_pin)
};

static struct lpuart_data lpuart_data;

#define INT_DRIVEN_API_SET(fp, func) \
	.fp = IS_ENABLED(CONFIG_NRF_SW_LPUART_INT_DRIVEN) ? func : NULL


static const struct uart_driver_api lpuart_api = {
	.callback_set = api_callback_set,
	.tx = api_tx,
	.tx_abort = api_tx_abort,
	.rx_enable = api_rx_enable,
	.rx_buf_rsp = api_rx_buf_rsp,
	.rx_disable = api_rx_disable,
	.poll_in = api_poll_in,
	.poll_out = api_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = api_configure,
	.config_get = api_config_get,
#endif
#if CONFIG_UART_INTERRUPT_DRIVEN
	INT_DRIVEN_API_SET(fifo_fill, api_fifo_fill),
	INT_DRIVEN_API_SET(fifo_read, api_fifo_read),
	INT_DRIVEN_API_SET(irq_tx_enable, api_irq_tx_enable),
	INT_DRIVEN_API_SET(irq_tx_disable, api_irq_tx_disable),
	INT_DRIVEN_API_SET(irq_tx_ready, api_irq_tx_ready),
	INT_DRIVEN_API_SET(irq_rx_enable, api_irq_rx_enable),
	INT_DRIVEN_API_SET(irq_rx_disable, api_irq_rx_disable),
	INT_DRIVEN_API_SET(irq_tx_complete, api_irq_tx_complete),
	INT_DRIVEN_API_SET(irq_rx_ready, api_irq_rx_ready),
	INT_DRIVEN_API_SET(irq_err_enable, api_irq_err_enable),
	INT_DRIVEN_API_SET(irq_err_disable, api_irq_err_disable),
	INT_DRIVEN_API_SET(irq_is_pending, api_irq_is_pending),
	INT_DRIVEN_API_SET(irq_update, api_irq_update),
	INT_DRIVEN_API_SET(irq_callback_set, api_irq_callback_set)
#endif
};

#define GPIO_HAS_PIN(gpio_node, pin_prop)				  \
	(DT_PROP(gpio_node, port) == (DT_INST_PROP(0, pin_prop) >> 5))
#define CHECK_GPIOTE_IRQ_PRIORITY(gpio_node)				  \
	BUILD_ASSERT((!GPIO_HAS_PIN(gpio_node, req_pin) &&		  \
		      !GPIO_HAS_PIN(gpio_node, rdy_pin)) ||		  \
		     DT_IRQ(DT_PARENT(DT_NODELABEL(lpuart)), priority) == \
		     DT_IRQ(GPIOTE_NODE(gpio_node), priority),		  \
		     "UARTE and GPIOTE interrupt priority must match.");

DT_FOREACH_STATUS_OKAY(nordic_nrf_gpio, CHECK_GPIOTE_IRQ_PRIORITY)

DEVICE_DT_DEFINE(DT_NODELABEL(lpuart), lpuart_init, NULL,
	      &lpuart_data, &lpuart_config,
	      POST_KERNEL, CONFIG_NRF_SW_LPUART_INIT_PRIORITY,
	      &lpuart_api);
