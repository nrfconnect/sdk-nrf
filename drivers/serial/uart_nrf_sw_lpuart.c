/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <drivers/uart.h>
#include <drivers/gpio.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>
#include <logging/log.h>
#include <sys/onoff.h>
#include <drivers/clock_control/nrf_clock_control.h>

LOG_MODULE_REGISTER(lpuart, CONFIG_NRF_SW_LPUART_LOG_LEVEL);

/* Structure describing bidirectional pin. */
struct lpuart_bidir_gpio {
	struct gpio_callback callback;

	/* gpio device. */
	const struct device *port;

	/* pin number - within port. */
	gpio_pin_t pin;

	/* absolute pin number. */
	uint8_t nrf_pin;

	/* gpiote channel allocated for that pin. */
	uint8_t ch;

	/* True if pin is used to request TX transfer. False if pin in idle is
	 * an input armed to detect transfer requestes.
	 */
	bool req;
};

/* States. */
enum rx_state {
	/* RX is disabled. */
	RX_OFF,

	/* RX is in low power, idle state with pin detection armed. */
	RX_IDLE,

	/* RX is in active state, receiver is running. */
	RX_ACTIVE,

	/* RX is transitioning to from active idle state. */
	RX_TO_IDLE,

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
	struct lpuart_bidir_gpio req_pin;

	/* Response pin. */
	struct lpuart_bidir_gpio rdy_pin;

	/* Timer used for TX timeouting. */
	struct k_timer tx_timer;

	/* Buffer used for poll out. */
	atomic_t txbyte;

	/* Current TX buffer. */
	const uint8_t *tx_buf;
	size_t tx_len;

	/* Set to true if physical transfer is started. */
	bool tx_active;

	/* Current RX buffer. */
	uint8_t *rx_buf;
	size_t rx_len;

	int32_t rx_timeout;

	uart_callback_t user_callback;
	void *user_data;

	/* RX state */
	enum rx_state rx_state;

	/* Set to true if request has been detected. */
	bool rx_req;

	struct onoff_client rx_clk_cli;

#if CONFIG_NRF_SW_LPUART_INT_DRIVEN
	struct lpuart_int_driven int_driven;
#endif
};

/* Configuration structured. */

struct lpuart_pin_config {
	const char *port_name;
	gpio_pin_t pin;
	uint8_t nrf_pin;
};

struct lpuart_config {
	const char *uart_name;
	struct lpuart_pin_config req;
	struct lpuart_pin_config rdy;
};

static inline struct lpuart_data *get_dev_data(const struct device *dev)
{
	return dev->data;
}

static inline const struct lpuart_config *get_dev_config(const struct device *dev)
{
	return dev->config;
}

static void ctrl_pin_set(const struct lpuart_bidir_gpio *io, bool force)
{
	if (force) {
		nrf_gpio_pin_set(io->nrf_pin);
		nrf_gpiote_te_default(NRF_GPIOTE, io->ch);
		nrf_gpio_cfg_output(io->nrf_pin);
		return;
	}

	int key = irq_lock();

	/* note that there is still a very small chance that if ZLI is used
	 * then it may be interrupted by ZLI and if during that time receiver
	 * clears the pin and sets it high again we may miss it.
	 * This might be solved in the next iteration but it will required
	 * extension to the GPIO driver to use LATCH for sensing. In case of
	 * sensing it is possible to switch from output to input in single
	 * operation (change or NRF_GPIO->PIN_CNF register).
	 */
	nrf_gpiote_te_default(NRF_GPIOTE, io->ch);
	nrf_gpiote_event_configure(NRF_GPIOTE, io->ch, io->nrf_pin,
					NRF_GPIOTE_POLARITY_HITOLO);

	nrf_gpio_cfg_input(io->nrf_pin, NRF_GPIO_PIN_PULLUP);
	nrf_gpiote_event_enable(NRF_GPIOTE, io->ch);

	irq_unlock(key);
}

/* Sets pin to output and sets low state. */
static void ctrl_pin_clear(const struct lpuart_bidir_gpio *io)
{
	nrf_gpio_pin_clear(io->nrf_pin);
	nrf_gpiote_te_default(NRF_GPIOTE, io->ch);
	nrf_gpio_cfg_output(io->nrf_pin);
}

/* Sets pin to idle state. In case of request pin it means out,low and in case
 * of response pin it means in, nopull, low to high detection enabled.
 */
static void ctrl_pin_idle(const struct lpuart_bidir_gpio *io)
{
	if (io->req) {
		ctrl_pin_clear(io);
		return;
	}

	nrf_gpiote_te_default(NRF_GPIOTE, io->ch);
	nrf_gpiote_event_configure(NRF_GPIOTE, io->ch, io->nrf_pin,
					NRF_GPIOTE_POLARITY_LOTOHI);

	nrf_gpio_cfg_input(io->nrf_pin, NRF_GPIO_PIN_NOPULL);
	nrf_gpiote_event_enable(NRF_GPIOTE, io->ch);
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

	err = uart_rx_enable(data->uart, data->rx_buf,
				data->rx_len, data->rx_timeout);
	__ASSERT(err == 0, "RX: Enabling failed (err:%d)", err);

	/* Ready. Confirm by toggling the pin. */
	ctrl_pin_clear(&data->rdy_pin);
	ctrl_pin_set(&data->rdy_pin, false);
	LOG_DBG("RX: Ready");
	data->rx_req = false;
	data->rx_state = RX_ACTIVE;
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
	if (IS_ENABLED(CONFIG_NRF_SW_LPUART_HFXO_ON_RX)) {
		rx_hfclk_request(data);
	} else {
		activate_rx(data);
	}
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

	ctrl_pin_idle(&data->rdy_pin);
	if (nrf_gpio_pin_read(data->rdy_pin.nrf_pin)) {
		LOG_DBG("RX: Request pending while deactivating");
		/* pin is set high, another request pending. */
		nrf_gpiote_event_clear(NRF_GPIOTE,
				     nrf_gpiote_in_event_get(data->rdy_pin.ch));
		data->rx_req = true;
	}

	/* abort rx */
	data->rx_state = RX_TO_IDLE;
	err = uart_rx_disable(data->uart);
	if (err < 0 && err != -EFAULT) {
		LOG_ERR("RX: Failed to disable (err: %d)", err);
	}
}

static void tx_complete(struct lpuart_data *data)
{
	ctrl_pin_idle(&data->req_pin);
	data->tx_buf = NULL;
	data->tx_active = false;
}

static void on_req_pin_change(struct lpuart_data *data)
{
	int key;
	const uint8_t *buf;
	size_t len;
	int err;

	if (data->tx_buf == NULL) {
		LOG_WRN("TX: request confirmed but no data to send");
		tx_complete(data);
		/* aborted */
		return;
	}

	LOG_DBG("TX: Confirmed, starting.");
	ctrl_pin_set(&data->req_pin, true);
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

static void on_rdy_pin_change(struct lpuart_data *data)
{
	if (nrf_gpiote_event_polarity_get(NRF_GPIOTE, data->rdy_pin.ch)
		== NRF_GPIOTE_POLARITY_LOTOHI) {
		__ASSERT_NO_MSG(data->rx_state != RX_ACTIVE);

		LOG_DBG("RX: Request detected.");
		data->rx_req = true;
		if (data->rx_state == RX_IDLE) {
			start_rx_activation(data);
		}
	} else {
		__ASSERT_NO_MSG(data->rx_state == RX_ACTIVE);

		LOG_DBG("RX: End detected.");
		deactivate_rx(data);
	}
}

static void gpio_handler(const struct device *port,
			struct gpio_callback *cb,
			gpio_port_pins_t pins)
{
	const struct lpuart_bidir_gpio *io =
		CONTAINER_OF(cb, struct lpuart_bidir_gpio, callback);
	struct lpuart_data *data;

	if (io->req) {
		data = CONTAINER_OF(io, struct lpuart_data, req_pin);
		on_req_pin_change(data);
		return;
	}

	data = CONTAINER_OF(io, struct lpuart_data, rdy_pin);
	on_rdy_pin_change(data);
}

static int ctrl_pin_configure(struct lpuart_bidir_gpio *io,
			      const struct lpuart_pin_config *cfg, bool req)
{
	int err;
	size_t i;

	io->pin = cfg->pin;
	io->nrf_pin = cfg->nrf_pin;
	io->req = req;
	io->port = device_get_binding(cfg->port_name);
	if (!io->port) {
		return -ENODEV;
	}

	gpio_init_callback(&io->callback, gpio_handler, BIT(io->pin));

	err = gpio_pin_configure(io->port, io->pin, GPIO_INPUT);
	if (err < 0) {
		return err;
	}

	err = gpio_add_callback(io->port, &io->callback);
	if (err < 0) {
		return err;
	}

	err = gpio_pin_interrupt_configure(io->port, io->pin, req ?
				GPIO_INT_EDGE_FALLING : GPIO_INT_EDGE_RISING);
	if (err < 0) {
		return err;
	}

	/* It is a hacky way to determine which channel is used for given pin.
	 * GPIO driver implementation change may lead to break.
	 */
	for (i = 0; i < GPIOTE_CH_NUM; i++) {
		if (nrf_gpiote_event_pin_get(NRF_GPIOTE, i) == io->nrf_pin) {
			io->ch = i;
			break;
		}
	}
	__ASSERT(i < GPIOTE_CH_NUM, "Used channel not found");

	ctrl_pin_idle(io);

	LOG_DBG("Pin %d configured, gpiote ch:%d, mode:%s",
		io->nrf_pin, io->ch, req ? "req" : "rdy");
	return 0;
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
		     evt->data.rx.buf, evt->data.rx.offset, evt->data.rx.len);
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
		user_callback(dev, evt);
		break;

	case UART_RX_DISABLED:
		LOG_DBG("Rx disabled");
		__ASSERT_NO_MSG((data->rx_state != RX_IDLE) &&
			 (data->rx_state != RX_OFF));

		if (data->rx_state == RX_TO_IDLE) {
			/* Need to request new buffer since uart was disabled */
			evt->type = UART_RX_BUF_REQUEST;
		} else {
			data->rx_buf = NULL;
			data->rx_state = RX_OFF;
		}

		user_callback(dev, evt);
		break;
	case UART_RX_STOPPED:
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
	k_timer_start(&data->tx_timer, SYS_TIMEOUT_MS(timeout), K_NO_WAIT);

	ctrl_pin_set(&data->req_pin, false);

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
	bool pending_rx;
	int key;

	__ASSERT_NO_MSG(data->rx_state == RX_OFF);

	if (atomic_ptr_cas((atomic_ptr_t *)&data->rx_buf, NULL, buf) == false) {
		return -EBUSY;
	}

	data->rx_len = len;
	data->rx_timeout = timeout;
	data->rx_state = RX_IDLE;

	LOG_DBG("RX: Enabling");

	key = irq_lock();
	pending_rx = nrf_gpio_pin_read(data->rdy_pin.nrf_pin)
		     && (data->rx_state == RX_IDLE);
	irq_unlock(key);

	if (pending_rx) {
		start_rx_activation(data);
	}

	return 0;
}

static int api_rx_buf_rsp(const struct device *dev, uint8_t *buf, size_t len)
{
	struct lpuart_data *data = get_dev_data(dev);

	__ASSERT_NO_MSG((data->rx_state != RX_OFF) &&
		 (data->rx_state != RX_TO_OFF));

	if (data->rx_state == RX_TO_IDLE) {
		data->rx_buf = buf;
		data->rx_len = len;

		if (data->rx_req) {
			LOG_DBG("RX: Pending request. Activating RX");
			start_rx_activation(data);
		} else {
			data->rx_state = RX_IDLE;
			LOG_DBG("RX: Idle");
		}

		return 0;
	}

	return uart_rx_buf_rsp(data->uart, buf, len);
}

static int api_rx_disable(const struct device *dev)
{
	struct lpuart_data *data = get_dev_data(dev);

	data->rx_state = RX_TO_OFF;

	return uart_rx_disable(data->uart);
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
					sizeof(data->int_driven.rxbuf), 1);
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
	if (!int_driven_rd_available(data) && data->rx_state == RX_TO_IDLE) {
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

	data->uart = device_get_binding(cfg->uart_name);
	if (data->uart == NULL) {
		return -ENODEV;
	}

	err = ctrl_pin_configure(&data->req_pin, &cfg->req, true);
	if (err < 0) {
		return -EINVAL;
	}

	err = ctrl_pin_configure(&data->rdy_pin, &cfg->rdy, false);
	if (err < 0) {
		return -EINVAL;
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
				sizeof(data->int_driven.rxbuf), 1);
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

#define DT_DRV_COMPAT nordic_nrf_sw_lpuart

#define GPIO(id) DT_NODELABEL(gpio##id)

#define GET_PORT(pin_prop) \
	COND_CODE_1(DT_NODE_EXISTS(GPIO(1)), \
		    ((DT_INST_PROP(0, pin_prop) >= 32) ? \
			DT_LABEL(GPIO(1)) : DT_LABEL(GPIO(0))), \
		    (DT_LABEL(GPIO(0))))

#define LPUART_PIN_CFG_INITIALIZER(pin_prop) \
	{ \
		.port_name = GET_PORT(pin_prop), \
		.pin = DT_INST_PROP(0, pin_prop) & 0x1F, \
		.nrf_pin = DT_INST_PROP(0, pin_prop) \
	}

static const struct lpuart_config lpuart_config = {
	.uart_name = DT_INST_BUS_LABEL(0),
	.req = LPUART_PIN_CFG_INITIALIZER(req_pin),
	.rdy = LPUART_PIN_CFG_INITIALIZER(rdy_pin)
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
	.configure = api_configure,
	.config_get = api_config_get,
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

DEVICE_DEFINE(lpuart, "LPUART", lpuart_init, device_pm_control_nop,
	      &lpuart_data, &lpuart_config,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	      &lpuart_api);
