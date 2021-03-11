/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <ctype.h>
#include <logging/log.h>
#include <drivers/uart.h>
#include <string.h>
#include <init.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>

LOG_MODULE_REGISTER(at_host, CONFIG_SLM_LOG_LEVEL);

#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_tcp_proxy.h"
#include "slm_at_udp_proxy.h"
#if defined(CONFIG_SLM_FTPC)
#include "slm_at_ftp.h"
#endif

#define OK_STR		"\r\nOK\r\n"
#define ERROR_STR	"\r\nERROR\r\n"
#define FATAL_STR	"FATAL ERROR\r\n"
#define OVERFLOW_STR	"Buffer overflow\r\n"
#define SLM_SYNC_STR	"Ready\r\n"

/** The maximum allowed length of an AT command passed through the SLM
 *  The space is allocated statically. This limit is in turn limited by
 *  Modem library's NRF_MODEM_AT_MAX_CMD_SIZE */
#define AT_MAX_CMD_LEN	4096

#define UART_RX_BUF_NUM         2
#define UART_RX_LEN             256
#define UART_RX_TIMEOUT_MS      1
#define UART_ERROR_DELAY_MS     500
#define UART_RX_MARGIN_MS       10

#define DATAMODE_SIZE_LIMIT_MAX	1024	/* byte */
#define DATAMODE_TIME_LIMIT_MAX	10000	/* msec */

/** @brief Termination Modes. */
enum term_modes {
	MODE_NULL_TERM, /**< Null Termination */
	MODE_CR,        /**< CR Termination */
	MODE_LF,        /**< LF Termination */
	MODE_CR_LF,     /**< CR+LF Termination */
	MODE_COUNT      /* Counter of term_modes */
};

static enum term_modes term_mode;
static const struct device *uart_dev;
static uint8_t at_buf[AT_MAX_CMD_LEN];
static size_t at_buf_len;
static bool at_buf_overflow;
static bool datamode_active;
static bool datamode_off_pending;
static slm_data_mode_handler_t datamode_handler;
static int64_t rx_start;
static struct k_work raw_send_work;
static struct k_work cmd_send_work;
static struct k_delayed_work uart_recovery_work;
static bool uart_recovery_pending;

static uint8_t uart_rx_buf[UART_RX_BUF_NUM][UART_RX_LEN];
static uint8_t *next_buf = uart_rx_buf[1];
static uint8_t *uart_tx_buf;

static K_SEM_DEFINE(tx_done, 0, 1);

/* global functions defined in different files */
int slm_at_parse(const char *at_cmd);
int slm_at_init(void);
void slm_at_uninit(void);

/* global variable used across different files */
struct at_param_list at_param_list;         /* For AT parser */
char rsp_buf[CONFIG_SLM_SOCKET_RX_MAX * 2]; /* SLM URC and socket data */
uint8_t rx_data[CONFIG_SLM_SOCKET_RX_MAX];  /* Socket RX raw data */
uint16_t datamode_size_limit;               /* Send trigger by size in data mode */
uint16_t datamode_time_limit;               /* Send trigger by time in data mode */

void rsp_send(const uint8_t *str, size_t len)
{
	int ret;

	if (len == 0) {
		return;
	}

	k_sem_take(&tx_done, K_FOREVER);

	uart_tx_buf = k_malloc(len);
	if (uart_tx_buf == NULL) {
		LOG_WRN("No ram buffer");
		k_sem_give(&tx_done);
		return;
	}

	LOG_HEXDUMP_DBG(str, len, "TX");

	memcpy(uart_tx_buf, str, len);
	ret = uart_tx(uart_dev, uart_tx_buf, len, SYS_FOREVER_MS);
	if (ret) {
		LOG_WRN("uart_tx failed: %d", ret);
		k_free(uart_tx_buf);
		k_sem_give(&tx_done);
	}
}

int enter_datamode(slm_data_mode_handler_t handler)
{
	if (handler == NULL || datamode_handler != NULL) {
		LOG_INF("Invalid, not enter datamode");
		return -EINVAL;
	}

	datamode_handler = handler;
	datamode_active = true;
	LOG_INF("Enter datamode");

	return 0;
}

bool exit_datamode(void)
{
	int err;

	if (datamode_active) {
		/* reset UART to restore command mode */
		uart_rx_disable(uart_dev);
		k_sleep(K_MSEC(10));
		err = uart_rx_enable(uart_dev, uart_rx_buf[0],
				     sizeof(uart_rx_buf[0]),
				     UART_RX_TIMEOUT_MS);
		if (err) {
			LOG_ERR("UART RX failed: %d", err);
			rsp_send(FATAL_STR, sizeof(FATAL_STR) - 1);
		}
		rx_start = k_uptime_get();

		datamode_active = false;
		datamode_handler = NULL;
		LOG_INF("Exit datamode");
		return true;
	}

	return false;
}

int poweroff_uart(void)
{
	int err;

	uart_rx_disable(uart_dev);
	k_sleep(K_MSEC(100));
	err = device_set_power_state(uart_dev, DEVICE_PM_OFF_STATE, NULL, NULL);
	if (err) {
		LOG_ERR("Can't power off uart: %d", err);
	}

	return err;
}

int poweron_uart(void)
{
	int err;
	uint32_t current_state = 0;

	err = device_get_power_state(uart_dev, &current_state);
	if (err) {
		LOG_ERR("Device get power_state: %d", err);
		return err;
	}

	if (current_state != DEVICE_PM_ACTIVE_STATE) {
		device_set_power_state(uart_dev, DEVICE_PM_ACTIVE_STATE, NULL, NULL);
		k_sleep(K_MSEC(100));
		err = uart_rx_enable(uart_dev, uart_rx_buf[0], sizeof(uart_rx_buf[0]),
				     UART_RX_TIMEOUT_MS);
		if (err) {
			LOG_ERR("Cannot enable rx: %d", err);
			return err;
		}
		k_sem_give(&tx_done);
		rsp_send(SLM_SYNC_STR, sizeof(SLM_SYNC_STR)-1);
	}

	return 0;
}

bool check_uart_flowcontrol(void)
{
	int err = -EINVAL;
	struct uart_config cfg = {
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
	};

	err = uart_config_get(uart_dev, &cfg);
	if (err) {
		LOG_ERR("uart_config_get: %d", err);
		return false;
	}
	return (cfg.flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS);
}

int set_uart_baudrate(uint32_t baudrate)
{
	int err = -EINVAL;
	struct uart_config cfg;

	LOG_DBG("Set uart baudrate to: %d", baudrate);

	err = uart_config_get(uart_dev, &cfg);
	if (err != 0) {
		LOG_ERR("uart_config_get: %d", err);
		return err;
	}
	cfg.baudrate = baudrate;
	err = uart_configure(uart_dev, &cfg);
	if (err != 0) {
		LOG_ERR("uart_configure: %d", err);
		return err;
	}

	return err;
}

int get_uart_baudrate(void)
{
	int err;
	struct uart_config cfg = {
		.baudrate = 0
	};

	err = uart_config_get(uart_dev, &cfg);
	if (err) {
		LOG_ERR("uart_config_get: %d", err);
		return -1;
	}
	return (int)cfg.baudrate;
}

bool verify_datamode_control(uint16_t size_limit, uint16_t time_limit)
{
	if (size_limit > DATAMODE_SIZE_LIMIT_MAX || time_limit > DATAMODE_TIME_LIMIT_MAX) {
		return false;
	}

	if (size_limit > 0  && time_limit > 0) {
		int baudrate;
		int min_time;

		baudrate = get_uart_baudrate();
		if (baudrate <= 0) {
			return false;
		}

		if (size_limit >= UART_RX_LEN) {
			min_time = UART_RX_LEN * (8 + 1 + 1) * 1000 / baudrate + UART_RX_MARGIN_MS;
		} else {
			min_time = size_limit * (8 + 1 + 1) * 1000 / baudrate + UART_RX_MARGIN_MS;
		}
		if (min_time > time_limit) {
			LOG_ERR("Invalid time_limit: %d, min: %d", time_limit, min_time);
			return false;
		}
	}

	return true;
}

static void response_handler(void *context, const char *response)
{
	int len = strlen(response);

	ARG_UNUSED(context);

	/* Forward the data over UART */
	if (len > 0) {
		rsp_send("\r\n", 2);
		rsp_send(response, len);
	}
}

static void uart_recovery(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	at_buf_overflow = false;
	err = uart_rx_enable(uart_dev, uart_rx_buf[0],
				sizeof(uart_rx_buf[0]), UART_RX_TIMEOUT_MS);
	if (err) {
		LOG_ERR("UART RX failed: %d", err);
		rsp_send(FATAL_STR, sizeof(FATAL_STR) - 1);
	}
	uart_recovery_pending = false;
	LOG_DBG("UART recovered");
}

static void raw_send(struct k_work *work)
{
	int err;

	ARG_UNUSED(work);

	LOG_HEXDUMP_DBG(at_buf, at_buf_len, "RX");

	if (datamode_handler) {
		(void)datamode_handler(at_buf, at_buf_len);
	} else {
		LOG_WRN("data dropped in data mode");
	}

	err = uart_rx_enable(uart_dev, uart_rx_buf[0],
			     sizeof(uart_rx_buf[0]), UART_RX_TIMEOUT_MS);
	if (err) {
		LOG_ERR("UART RX failed: %d", err);
	}
	rx_start = k_uptime_get();
	at_buf_len = 0;
}

static void inactivity_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	LOG_INF("time limit reached, size: %d", at_buf_len);

	if (at_buf_len > 0) {
		uart_rx_disable(uart_dev);
		k_work_submit(&raw_send_work);
	}
}

K_TIMER_DEFINE(inactivity_timer, inactivity_timer_handler, NULL);

static void silence_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	/* Drop pending data unconditionally */
	if (datamode_time_limit > 0) {
		k_timer_stop(&inactivity_timer);
	}
	at_buf_len = 0;

	/* quit datamode */
	(void)exit_datamode();
	slm_tcp_set_datamode_off();
	slm_udp_set_datamode_off();
#if defined(CONFIG_SLM_FTPC)
	slm_ftp_set_datamode_off();
#endif
	datamode_off_pending = false;
	/* send URC */
	rsp_send(OK_STR, sizeof(OK_STR) - 1);
}

K_TIMER_DEFINE(silence_timer, silence_timer_handler, NULL);

static int raw_rx_handler(const uint8_t *data, int datalen)
{
	const char *quit_str = CONFIG_SLM_DATAMODE_TERMINATOR;
	int quit_str_len = strlen(quit_str);
	int64_t silence = CONFIG_SLM_DATAMODE_SILENCE * MSEC_PER_SEC;

	/* First, check conditions for quitting datamode */
	if (silence > k_uptime_delta(&rx_start)) {
		if (datamode_off_pending) {
			/* quit procedure aborted */
			k_timer_stop(&silence_timer);
			datamode_off_pending = false;
			memcpy(at_buf + at_buf_len, quit_str, quit_str_len);
			at_buf_len += quit_str_len;
			LOG_INF("datamode off cancelled");
		}
	} else {
		/* leading silence confirmed */
		if (datalen == quit_str_len && strncmp(data, quit_str, quit_str_len) == 0) {
			datamode_off_pending = true;
			/* check subordinate silence */
			k_timer_start(&silence_timer, K_SECONDS(CONFIG_SLM_DATAMODE_SILENCE),
				      K_NO_WAIT);
			LOG_INF("datamode off pending");
			rx_start = k_uptime_get();
			return 0;
		}
	}

	/* Second, check conditions for sending */
	if (datamode_time_limit > 0) {
		k_timer_stop(&inactivity_timer);
	}

	memcpy(at_buf + at_buf_len, data, datalen);
	at_buf_len += datalen;

	if (datamode_size_limit > 0 && at_buf_len >= datamode_size_limit) {
		LOG_INF("size limit reached, size: %d", at_buf_len);
		goto transit;
	}

	/* start/restart inactivity timer */
	if (datamode_time_limit > 0) {
		k_timer_start(&inactivity_timer, K_MSEC(datamode_time_limit), K_NO_WAIT);
	}

	/* Third, check the condition of no size/time limit */
	if (datamode_size_limit > 0 || datamode_time_limit > 0) {
		rx_start = k_uptime_get();
		return 0;
	}

transit:
	uart_rx_disable(uart_dev);
	k_work_submit(&raw_send_work);

	return 0;
}

static void cmd_send(struct k_work *work)
{
	char str[32];
	enum at_cmd_state state;
	int err;

	ARG_UNUSED(work);

	if (at_buf_overflow) {
		rsp_send(OVERFLOW_STR, sizeof(OVERFLOW_STR) - 1);
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}

	LOG_HEXDUMP_DBG(at_buf, at_buf_len, "RX");

	err = slm_at_parse(at_buf);
	if (err == 0) {
		rsp_send(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	} else if (err == -ESHUTDOWN) {
		/*Entered IDLE or UART Power Off */
		return;
	} else if (err != -ENOENT) {
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}

	/* Send to modem */
	err = at_cmd_write(at_buf, at_buf, sizeof(at_buf), &state);
	if (err < 0) {
		LOG_ERR("AT command error: %d", err);
		state = AT_CMD_ERROR;
	}

	if (strlen(at_buf) > 0) {
		rsp_send("\r\n", 2);
		rsp_send(at_buf, strlen(at_buf));
	}

	switch (state) {
	case AT_CMD_OK:
		rsp_send(OK_STR, sizeof(OK_STR) - 1);
		break;
	case AT_CMD_ERROR:
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		break;
	case AT_CMD_ERROR_CMS:
		sprintf(str, "\r\n+CMS ERROR: %d\r\n", err);
		rsp_send(str, strlen(str));
		break;
	case AT_CMD_ERROR_CME:
		sprintf(str, "\r\n+CME ERROR: %d\r\n", err);
		rsp_send(str, strlen(str));
		break;
	default:
		break;
	}

done:
	at_buf_overflow = false;
	err = uart_rx_enable(uart_dev, uart_rx_buf[0],
				sizeof(uart_rx_buf[0]), UART_RX_TIMEOUT_MS);
	if (err) {
		LOG_ERR("UART RX failed: %d", err);
		rsp_send(FATAL_STR, sizeof(FATAL_STR) - 1);
	}
	/* prepare for transition to datamode, just in case */
	rx_start = k_uptime_get();
	at_buf_len = 0;
}

static int cmd_rx_handler(uint8_t character)
{
	static bool inside_quotes;
	static size_t at_cmd_len;

	/* Handle control characters */
	switch (character) {
	case 0x08: /* Backspace. */
		/* Fall through. */
	case 0x7F: /* DEL character */
		if (at_cmd_len > 0) {
			at_cmd_len--;
		}
		return 0;
	}

	/* Handle termination characters, if outside quotes. */
	if (!inside_quotes) {
		switch (character) {
		case '\0':
			if (term_mode == MODE_NULL_TERM) {
				goto send;
			}
			LOG_WRN("Ignored null; would terminate string early.");
			return 0;
		case '\r':
			if (term_mode == MODE_CR) {
				goto send;
			}
			break;
		case '\n':
			if (term_mode == MODE_LF) {
				goto send;
			}
			if (term_mode == MODE_CR_LF &&
			    at_cmd_len > 0 &&
			    at_buf[at_cmd_len - 1] == '\r') {
				goto send;
			}
			break;
		}
	}

	/* Write character to AT buffer */
	at_buf[at_cmd_len] = character;
	at_cmd_len++;

	/* Detect AT command buffer overflow, leaving space for null */
	if (at_cmd_len > sizeof(at_buf) - 1) {
		LOG_ERR("Buffer overflow");
		at_cmd_len--;
		at_buf_overflow = true;
		goto send;
	}

	/* Handle special written character */
	if (character == '"') {
		inside_quotes = !inside_quotes;
	}

	return 0;

send:
	uart_rx_disable(uart_dev);

	at_buf[at_cmd_len] = '\0';
	at_buf_len = at_cmd_len;
	k_work_submit(&cmd_send_work);

	inside_quotes = false;
	at_cmd_len = 0;
	if (at_buf_overflow) {
		return -1;
	}
	return 0;
}

static void uart_callback(const struct device *dev, struct uart_event *evt,
			  void *user_data)
{
	static bool enable_rx_retry;

	ARG_UNUSED(dev);

	int err;
	static uint16_t pos;

	ARG_UNUSED(user_data);

	switch (evt->type) {
	case UART_TX_DONE:
		k_free(uart_tx_buf);
		k_sem_give(&tx_done);
		break;
	case UART_TX_ABORTED:
		k_free(uart_tx_buf);
		k_sem_give(&tx_done);
		LOG_INF("TX_ABORTED");
		break;
	case UART_RX_RDY:
		if (datamode_active) {
			raw_rx_handler(&(evt->data.rx.buf[pos]),
					evt->data.rx.len);
			return;
		}
		for (int i = pos; i < (pos + evt->data.rx.len); i++) {
			err = cmd_rx_handler(evt->data.rx.buf[i]);
			if (err) {
				return;
			}
		}
		pos += evt->data.rx.len;
		break;
	case UART_RX_BUF_REQUEST:
		pos = 0;
		err = uart_rx_buf_rsp(uart_dev, next_buf,
					sizeof(uart_rx_buf[0]));
		if (err) {
			LOG_WRN("UART RX buf rsp: %d", err);
		}
		break;
	case UART_RX_BUF_RELEASED:
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
			k_delayed_work_submit(&uart_recovery_work,
				K_MSEC(UART_ERROR_DELAY_MS));
			enable_rx_retry = false;
			uart_recovery_pending = true;
		}
		break;
	default:
		break;
	}
}

int slm_at_host_init(void)
{
	int err;
	uint32_t start_time;

	/* Initialize the UART module */
#if defined(CONFIG_SLM_CONNECT_UART_0)
	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart0)));
#elif defined(CONFIG_SLM_CONNECT_UART_2)
	uart_dev = device_get_binding(DT_LABEL(DT_NODELABEL(uart2)));
#else
	LOG_ERR("Unsupported UART instance");
	return -EINVAL;
#endif
	if (uart_dev == NULL) {
		LOG_ERR("Cannot bind UART device\n");
		return -EINVAL;
	}
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
	/* Register async handling callback */
	err = uart_callback_set(uart_dev, uart_callback, NULL);
	if (err) {
		LOG_ERR("Cannot set callback: %d", err);
		return -EFAULT;
	}
	/* Power on UART module */
	device_set_power_state(uart_dev, DEVICE_PM_ACTIVE_STATE,
				NULL, NULL);
	term_mode = CONFIG_SLM_AT_HOST_TERMINATION;
	err = uart_rx_enable(uart_dev, uart_rx_buf[0],
				sizeof(uart_rx_buf[0]), UART_RX_TIMEOUT_MS);
	if (err) {
		LOG_ERR("Cannot enable rx: %d", err);
		return -EFAULT;
	}

	err = at_notif_register_handler(NULL, response_handler);
	if (err) {
		LOG_ERR("Can't register handler err=%d", err);
		return err;
	}

	/* Initialize AT Parser */
	err = at_params_list_init(&at_param_list, CONFIG_SLM_AT_MAX_PARAM);
	if (err) {
		LOG_ERR("Failed to init AT Parser: %d", err);
		return err;
	}

	datamode_active = false;
	datamode_time_limit = 0;
	datamode_size_limit = 0;
	datamode_handler = NULL;

	err = slm_at_init();
	if (err) {
		return -EFAULT;
	}

	k_work_init(&raw_send_work, raw_send);
	k_work_init(&cmd_send_work, cmd_send);
	k_delayed_work_init(&uart_recovery_work, uart_recovery);
	k_sem_give(&tx_done);
	rsp_send(SLM_SYNC_STR, sizeof(SLM_SYNC_STR)-1);

	LOG_DBG("at_host init done");
	return err;
}

void slm_at_host_uninit(void)
{
	int err;

	if (datamode_active) {
		k_timer_stop(&silence_timer);
		k_timer_stop(&inactivity_timer);
	}
	datamode_active = false;
	datamode_time_limit = 0;
	datamode_size_limit = 0;
	datamode_handler = NULL;

	slm_at_uninit();

	err = at_notif_deregister_handler(NULL, response_handler);
	if (err) {
		LOG_WRN("Can't deregister handler: %d", err);
	}

	/* Power off UART module */
	uart_rx_disable(uart_dev);
	k_sleep(K_MSEC(100));
	err = device_set_power_state(uart_dev, DEVICE_PM_OFF_STATE,
				NULL, NULL);
	if (err) {
		LOG_WRN("Can't power off uart: %d", err);
	}

	LOG_DBG("at_host uninit done");
}
