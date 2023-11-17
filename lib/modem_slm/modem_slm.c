/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include <hal/nrf_gpio.h>
#include <modem/modem_slm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mdm_slm, CONFIG_MODEM_SLM_LOG_LEVEL);

BUILD_ASSERT(CONFIG_MODEM_SLM_WAKEUP_PIN >= 0, "Wake up pin not configured");

#define UART_RX_BUF_NUM         2
#define UART_RX_LEN             CONFIG_MODEM_SLM_DMA_MAXLEN
#define UART_RX_TIMEOUT_US      2000
#define UART_ERROR_DELAY_MS     500
#define UART_RX_MARGIN_MS       10

/* SLM has formatted AT response based on TS 27.007 */
#define AT_CMD_OK_STR    "\r\nOK\r\n"
#define AT_CMD_ERROR_STR "\r\nERROR\r\n"
#define AT_CMD_CMS_STR   "\r\n+CMS ERROR:"
#define AT_CMD_CME_STR   "\r\n+CME ERROR:"

static const struct device *uart_dev = DEVICE_DT_GET(DT_CHOSEN(ncs_slm_uart));
static uint8_t uart_rx_buf[UART_RX_BUF_NUM][UART_RX_LEN];
static uint8_t *next_buf;
static uint8_t *uart_tx_buf;
static K_SEM_DEFINE(tx_done, 0, 1);
static bool uart_recovery_pending;
static struct k_work_delayable uart_recovery_work;

static slm_data_handler_t data_handler;
static K_SEM_DEFINE(at_rsp, 0, 1);
static enum at_cmd_state slm_at_state;

RING_BUF_DECLARE(slm_data_rb, SLM_AT_CMD_RESPONSE_MAX_LEN);
static struct k_work slm_data_work;

#if DT_HAS_CHOSEN(ncs_slm_gpio)
static const struct device *gpio_dev = DEVICE_DT_GET(DT_CHOSEN(ncs_slm_gpio));
#else
static const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
#endif
static struct k_work_delayable gpio_wakeup_work;
static slm_ind_handler_t ind_handler;

#if defined(CONFIG_MODEM_SLM_SHELL)
static struct shell *global_shell;
static const char at_usage_str[] = "Usage: slm <at_command>";
#endif

/* global functions defined in different files */
void slm_monitor_dispatch(const char *notif);

static void gpio_wakeup_wk(struct k_work *work)
{
	ARG_UNUSED(work);

	if (gpio_pin_set(gpio_dev, CONFIG_MODEM_SLM_WAKEUP_PIN, 0) != 0) {
		LOG_WRN("GPIO set error");
	}
	LOG_DBG("Stop wake-up");
}

static void slm_data_wk(struct k_work *work)
{
	uint8_t *data = NULL;
	int size_send;

	ARG_UNUSED(work);

	/* NOTE ring_buf_get_claim() might not return full size */
	do {
		size_send = ring_buf_get_claim(&slm_data_rb, &data, SLM_AT_CMD_RESPONSE_MAX_LEN);
		if (data != NULL && size_send > 0) {
#if defined(CONFIG_MODEM_SLM_SHELL)
			shell_print(global_shell, "%.*s", size_send, (char *)data);
#endif
			if (data_handler) {
				data_handler(data, size_send);
			}
			(void)ring_buf_get_finish(&slm_data_rb, size_send);
		} else {
			break;
		}
	} while (true);
}

static void uart_recovery_wk(struct k_work *work)
{
	ARG_UNUSED(work);

	next_buf = uart_rx_buf[1];
	int err = uart_rx_enable(uart_dev, uart_rx_buf[0], sizeof(uart_rx_buf[0]),
				 UART_RX_TIMEOUT_US);

	if (err && err != -EBUSY) {
		LOG_ERR("UART recovery failed: %d", err);
	} else {
		LOG_DBG("UART recovered");
	}
	uart_recovery_pending = false;
}

static int uart_send(const uint8_t *buffer, size_t len)
{
	int ret;

	LOG_HEXDUMP_DBG(buffer, len, "TX");

	k_sem_take(&tx_done, K_FOREVER);

	if ((uint32_t)buffer < CONFIG_PM_SRAM_BASE ||
	    (uint32_t)buffer > CONFIG_PM_SRAM_BASE + CONFIG_PM_SRAM_SIZE) {
		uart_tx_buf = k_malloc(len);
		if (uart_tx_buf == NULL) {
			LOG_WRN("No ram buffer");
			k_sem_give(&tx_done);
			return -ENOMEM;
		}
		memcpy(uart_tx_buf, buffer, len);
		ret = uart_tx(uart_dev, uart_tx_buf, len, SYS_FOREVER_US);
	} else {
		uart_tx_buf = NULL; /* if NULL, no operation by k_free() */
		ret = uart_tx(uart_dev, buffer, len, SYS_FOREVER_US);
	}
	if (ret) {
		LOG_WRN("uart_tx failed: %d", ret);
		k_free(uart_tx_buf);
		k_sem_give(&tx_done);
	}

	return ret;
}

/* Attempts to find AT responses in the UART buffer. */
static void parse_at_response(const char *data, size_t datalen)
{
	/* SLM AT responses are formatted based on TS 27.007. */
	static const char * const at_responses[] = {
		[AT_CMD_OK] = "\r\nOK\r\n",
		[AT_CMD_ERROR] = "\r\nERROR\r\n",
		[AT_CMD_ERROR_CMS] = "\r\n+CMS ERROR:",
		[AT_CMD_ERROR_CME] = "\r\n+CME ERROR:"
	};
	static size_t at_response_lens[ARRAY_SIZE(at_responses)];

	if (!at_response_lens[0]) {
		/* Initialize the AT response lengths array. */
		for (size_t at_state = 0; at_state != ARRAY_SIZE(at_responses); ++at_state) {
			at_response_lens[at_state] = strlen(at_responses[at_state]);
		}
	}
	/* Search the UART buffer in sequential order. */
	for (size_t i = 0; i != datalen; ++i) {
		/* Look for all the AT state responses. */
		for (size_t at_state = 0; at_state != ARRAY_SIZE(at_responses); ++at_state) {
			const char *at_response = at_responses[at_state];
			const size_t at_response_len = at_response_lens[at_state];

			if (i + at_response_len <= datalen
			&& !strncmp(data + i, at_response, at_response_len)) {
				/* Found a match. */
				slm_at_state = at_state;
				return;
			}
		}
	}
}

static void uart_rx_handler(const uint8_t *data, size_t datalen)
{
	int ret;

	LOG_HEXDUMP_DBG(data, datalen, "RX");

	/* save data to buffer */
	ret = ring_buf_put(&slm_data_rb, data, datalen);
	if (ret != datalen) {
		LOG_WRN("enqueue data error (%d, %d)", datalen, ret);
	}

	/* handle AT response */
	if (slm_at_state == AT_CMD_PENDING) {
		parse_at_response(data, datalen);
		if (slm_at_state != AT_CMD_PENDING) {
			k_work_submit(&slm_data_work);
			k_sem_give(&at_rsp);
		}
	/* handle AT unsolicited */
	} else {
		slm_monitor_dispatch((const char *)data);
		k_work_submit(&slm_data_work);
	}
}

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	int err;
	static uint16_t pos;
	static bool enable_rx_retry;

	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case UART_TX_DONE:
		k_free(uart_tx_buf);
		k_sem_give(&tx_done);
		LOG_DBG("TX_DONE");
		break;
	case UART_TX_ABORTED:
		k_free(uart_tx_buf);
		k_sem_give(&tx_done);
		LOG_INF("TX_ABORTED");
		break;
	case UART_RX_RDY:
		LOG_DBG("RX_RDY %d", evt->data.rx.len);
		uart_rx_handler(&(evt->data.rx.buf[pos]), evt->data.rx.len);
		pos += evt->data.rx.len;
		break;
	case UART_RX_BUF_REQUEST:
		LOG_DBG("RX_BUF_REQ");
		pos = 0;
		err = uart_rx_buf_rsp(uart_dev, next_buf, sizeof(uart_rx_buf[0]));
		if (err) {
			LOG_WRN("UART RX buf rsp: %d", err);
		}
		break;
	case UART_RX_BUF_RELEASED:
		LOG_DBG("RX_BUF_REL");
		next_buf = evt->data.rx_buf.buf;
		break;
	case UART_RX_STOPPED:
		LOG_WRN("RX_STOPPED (%d)", evt->data.rx_stop.reason);
		/* Retry automatically in case of UART ERROR interrupt */
		if (evt->data.rx_stop.reason != 0) {
			enable_rx_retry = true;
		}
		break;
	case UART_RX_DISABLED:
		LOG_DBG("RX_DISABLED");
		if (enable_rx_retry && !uart_recovery_pending) {
			k_work_schedule(&uart_recovery_work, K_MSEC(UART_RX_MARGIN_MS));
			enable_rx_retry = false;
			uart_recovery_pending = true;
		}
		break;
	default:
		break;
	}
}

static int uart_init(const struct device *uart_dev)
{
	int err;
	struct uart_config uart_conf;

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	err = uart_config_get(uart_dev, &uart_conf);
	if (err == 0) {
		LOG_INF("UART baud: %d d/p/s-bits: %d/%d/%d HWFC: %d",
			uart_conf.baudrate, uart_conf.data_bits, uart_conf.parity,
			uart_conf.stop_bits, uart_conf.flow_ctrl);
	}

	/* Wait for the UART line to become valid */
	uint32_t start_time = k_uptime_get_32();

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

	/* Register async handling callback */
	err = uart_callback_set(uart_dev, uart_callback, NULL);
	if (err) {
		LOG_ERR("Cannot set callback: %d", err);
		return -EFAULT;
	}

	/* Enable TX */
	k_sem_give(&tx_done);
	/* Enable RX */
	next_buf = uart_rx_buf[1];
	err = uart_rx_enable(uart_dev, uart_rx_buf[0], sizeof(uart_rx_buf[0]), UART_RX_TIMEOUT_US);
	if (err && err != -EBUSY) {
		LOG_ERR("UART RX failed: %d", err);
	}

	return err;
}

#if (CONFIG_MODEM_SLM_INDICATE_PIN >= 0)
static struct gpio_callback gpio_cb;

static void gpio_cb_func(const struct device *dev, struct gpio_callback *gpio_cb, uint32_t pins)
{
	if ((BIT(CONFIG_MODEM_SLM_INDICATE_PIN) & pins) == 0) {
		return;
	}

	if (k_work_delayable_is_pending(&gpio_wakeup_work)) {
		(void)k_work_cancel_delayable(&gpio_wakeup_work);
		(void)gpio_pin_set(gpio_dev, CONFIG_MODEM_SLM_WAKEUP_PIN, 0);
	}

	LOG_INF("Remote indication");
	if (ind_handler) {
		ind_handler();
	}
}
#endif  /* CONFIG_MODEM_SLM_INDICATE_PIN */

static int gpio_init(void)
{
	int err;

	if (!device_is_ready(gpio_dev)) {
		LOG_ERR("GPIO controller not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure(gpio_dev, CONFIG_MODEM_SLM_WAKEUP_PIN,
				 GPIO_OUTPUT_INACTIVE | GPIO_ACTIVE_LOW);
	if (err) {
		LOG_ERR("GPIO config error: %d", err);
		return err;
	}

#if (CONFIG_MODEM_SLM_INDICATE_PIN >= 0)
	err = gpio_pin_configure(gpio_dev, CONFIG_MODEM_SLM_INDICATE_PIN,
				 GPIO_INPUT | GPIO_PULL_UP | GPIO_ACTIVE_LOW);
	if (err) {
		LOG_ERR("GPIO config error: %d", err);
		return err;
	}

	gpio_init_callback(&gpio_cb, gpio_cb_func, BIT(CONFIG_MODEM_SLM_INDICATE_PIN));
	err = gpio_add_callback(gpio_dev, &gpio_cb);
	if (err) {
		LOG_WRN("GPIO add callback error: %d", err);
	}
	err = gpio_pin_interrupt_configure(gpio_dev, CONFIG_MODEM_SLM_INDICATE_PIN,
					   GPIO_INT_LEVEL_LOW);
	if (err) {
		LOG_WRN("GPIO enable callback error: %d", err);
	}
#endif

	return 0;
}

int modem_slm_init(slm_data_handler_t handler)
{
	int err;

	if (handler != NULL && data_handler != NULL) {
		LOG_ERR("Already initialized");
		return -EFAULT;
	}

	data_handler = handler;
	ind_handler = NULL;
	slm_at_state = AT_CMD_OK;

	err = gpio_init();
	if (err != 0) {
		LOG_WRN("Failed to init gpio");
		return err;
	}

	err = uart_init(uart_dev);
	if (err) {
		LOG_ERR("UART could not be initialized: %d", err);
		return -EFAULT;
	}

	k_work_init_delayable(&gpio_wakeup_work, gpio_wakeup_wk);
	k_work_init(&slm_data_work, slm_data_wk);
	k_work_init_delayable(&uart_recovery_work, uart_recovery_wk);

	return 0;
}

int modem_slm_uninit(void)
{
	uart_rx_disable(uart_dev);
	k_sleep(K_MSEC(10));

	gpio_pin_configure(gpio_dev, CONFIG_MODEM_SLM_WAKEUP_PIN, GPIO_DISCONNECTED);
#if (CONFIG_MODEM_SLM_INDICATE_PIN >= 0)
	gpio_remove_callback(gpio_dev, &gpio_cb);
	gpio_pin_interrupt_configure(gpio_dev, CONFIG_MODEM_SLM_INDICATE_PIN, GPIO_INT_DISABLE);
	gpio_pin_configure(gpio_dev, CONFIG_MODEM_SLM_INDICATE_PIN, GPIO_DISCONNECTED);
#endif

	data_handler = NULL;
	ind_handler = NULL;
	slm_at_state = AT_CMD_OK;

	return 0;
}

int modem_slm_register_ind(slm_ind_handler_t handler, bool wakeup)
{
#if (CONFIG_MODEM_SLM_INDICATE_PIN >= 0)
	ind_handler = handler;

	if (wakeup) {
		/*
		 * Due to errata 4, Always configure PIN_CNF[n].INPUT before PIN_CNF[n].SENSE.
		 * At this moment WAKEUP_PIN has already been configured as INPUT at init_gpio().
		 */
		nrf_gpio_cfg_sense_set(CONFIG_MODEM_SLM_INDICATE_PIN, NRF_GPIO_PIN_SENSE_LOW);
	}

	return 0;
#else
	return -EFAULT;
#endif
}

int modem_slm_wake_up(void)
{
	int err;

	if (k_work_delayable_is_pending(&gpio_wakeup_work)) {
		return 0;
	}

	LOG_INF("Start wake-up");

	err = gpio_pin_set(gpio_dev, CONFIG_MODEM_SLM_WAKEUP_PIN, 1);
	if (err) {
		LOG_ERR("GPIO set error: %d", err);
	} else {
		k_work_reschedule(&gpio_wakeup_work, K_MSEC(CONFIG_MODEM_SLM_WAKEUP_TIME));
	}

	return 0;
}

void modem_slm_reset_uart(void)
{
	int err;

	uart_rx_disable(uart_dev);
	k_sleep(K_MSEC(10));

	next_buf = uart_rx_buf[1];
	err = uart_rx_enable(uart_dev, uart_rx_buf[0], sizeof(uart_rx_buf[0]), UART_RX_TIMEOUT_US);
	if (err && err != -EBUSY) {
		LOG_ERR("UART RX failed: %d", err);
	}
}

int modem_slm_send_cmd(const char *const command, uint32_t timeout)
{
	int ret;

	slm_at_state = AT_CMD_PENDING;
	ret = uart_send(command, strlen(command));
	if (ret < 0) {
		return ret;
	}
	/* send AT command terminator */
#if defined(CONFIG_MODEM_SLM_CR_LF_TERMINATION)
	ret = uart_send("\r\n", 2);
#elif defined(CONFIG_MODEM_SLM_CR_TERMINATION)
	ret = uart_send("\r", 1);
#elif defined(CONFIG_MODEM_SLM_LF_TERMINATION)
	ret = uart_send("\n", 1);
#endif
	if (ret < 0) {
		return ret;
	}
	if (timeout != 0) {
		ret = k_sem_take(&at_rsp, K_SECONDS(timeout));
	} else {
		ret = k_sem_take(&at_rsp, K_FOREVER);
	}
	if (ret < 0 && ret != -EBUSY) {
		LOG_ERR("timeout");
		return ret;
	}

	return slm_at_state;
}

int modem_slm_send_data(const uint8_t *const data, size_t datalen)
{
	return uart_send(data, datalen);
}

#if defined(CONFIG_MODEM_SLM_SHELL)

int modem_slm_shell(const struct shell *shell, size_t argc, char **argv)
{
	global_shell = (struct shell *)shell;

	if (argc < 2) {
		shell_print(shell, "%s", at_usage_str);
		return 0;
	}

	return modem_slm_send_cmd((char *)argv[1], 0);
}

SHELL_CMD_REGISTER(slm, NULL, "SLM Shell", modem_slm_shell);

#endif /* CONFIG_MODEM_SLM_SHELL */
