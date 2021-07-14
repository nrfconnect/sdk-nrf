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
#include <sys/ring_buffer.h>
#include <string.h>
#include <init.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>

LOG_MODULE_REGISTER(at_host, CONFIG_SLM_LOG_LEVEL);

#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_fota.h"

#define OK_STR		"\r\nOK\r\n"
#define ERROR_STR	"\r\nERROR\r\n"
#define FATAL_STR	"FATAL ERROR\r\n"
#define SLM_SYNC_STR	"Ready\r\n"

/** The maximum allowed length of an AT command passed through the SLM
 *  The space is allocated statically. This limit is in turn limited by
 *  Modem library's NRF_MODEM_AT_MAX_CMD_SIZE */
#define AT_MAX_CMD_LEN          4096

#define UART_RX_BUF_NUM         2
#define UART_RX_LEN             256
#define UART_RX_TIMEOUT_MS      1
#define UART_ERROR_DELAY_MS     500
#define UART_RX_MARGIN_MS       10

static enum slm_operation_modes {
	SLM_AT_COMMAND_MODE,  /* AT command host or bridge */
	SLM_DATA_MODE,        /* Raw data sending */
} slm_operation_mode;

static const struct device *uart_dev;
static uint8_t at_buf[AT_MAX_CMD_LEN];
static uint16_t at_buf_len;
static bool at_buf_overflow;
static struct ring_buf data_rb;
static bool datamode_off_pending;
static bool datamode_rx_disabled;
static slm_datamode_handler_t datamode_handler;
static struct k_work raw_send_work;
static struct k_work cmd_send_work;

static uint8_t uart_rx_buf[UART_RX_BUF_NUM][UART_RX_LEN];
static uint8_t *next_buf = uart_rx_buf[1];
static uint8_t *uart_tx_buf;
static bool uart_recovery_pending;
static struct k_work_delayable uart_recovery_work;

static K_SEM_DEFINE(tx_done, 0, 1);

/* global functions defined in different files */
int slm_at_parse(const char *at_cmd);
int slm_at_init(void);
void slm_at_uninit(void);
int slm_setting_uart_save(void);

/* global variable used across different files */
struct at_param_list at_param_list;         /* For AT parser */
char rsp_buf[CONFIG_SLM_SOCKET_RX_MAX * 2]; /* SLM URC and socket data */
uint8_t rx_data[CONFIG_SLM_SOCKET_RX_MAX];  /* Socket RX raw data */
uint16_t datamode_time_limit;               /* Send trigger by time in data mode */

/* global variable defined in different files */
extern bool uart_configured;
extern struct uart_config slm_uart;

static int uart_send(const uint8_t *str, size_t len)
{
	int ret;

	k_sem_take(&tx_done, K_FOREVER);

	uart_tx_buf = k_malloc(len);
	if (uart_tx_buf == NULL) {
		LOG_WRN("No ram buffer");
		k_sem_give(&tx_done);
		return -ENOMEM;
	}

	memcpy(uart_tx_buf, str, len);
	ret = uart_tx(uart_dev, uart_tx_buf, len, SYS_FOREVER_MS);
	if (ret) {
		LOG_WRN("uart_tx failed: %d", ret);
		k_free(uart_tx_buf);
		k_sem_give(&tx_done);
	}

	return ret;
}

void rsp_send(const uint8_t *str, size_t len)
{
	if (len == 0) {
		return;
	}

	LOG_HEXDUMP_DBG(str, len, "TX");
	(void)uart_send(str, len);
}

static int uart_receive(void)
{
	int ret;

	ret = uart_rx_enable(uart_dev, uart_rx_buf[0], sizeof(uart_rx_buf[0]), UART_RX_TIMEOUT_MS);
	if (ret) {
		LOG_ERR("UART RX failed: %d", ret);
		rsp_send(FATAL_STR, sizeof(FATAL_STR) - 1);
		return ret;
	}
	at_buf_overflow = false;
	at_buf_len = 0;

	return 0;
}

static void uart_recovery(struct k_work *work)
{
	ARG_UNUSED(work);

	(void)uart_receive();
	uart_recovery_pending = false;
	LOG_DBG("UART recovered");
}

int enter_datamode(slm_datamode_handler_t handler)
{
	if (handler == NULL || datamode_handler != NULL) {
		LOG_INF("Invalid, not enter datamode");
		return -EINVAL;
	}

	ring_buf_init(&data_rb, sizeof(at_buf) / 2, at_buf);
	datamode_handler = handler;
	slm_operation_mode = SLM_DATA_MODE;
	LOG_INF("Enter datamode");

	return 0;
}

bool in_datamode(void)
{
	return (slm_operation_mode == SLM_DATA_MODE);
}

bool exit_datamode(bool response)
{
	if (slm_operation_mode == SLM_DATA_MODE) {
		ring_buf_reset(&data_rb);
		/* reset UART to restore command mode */
		uart_rx_disable(uart_dev);
		k_sleep(K_MSEC(10));
		(void)uart_receive();

		if (response) {
			strcpy(rsp_buf, OK_STR);
		} else {
			sprintf(rsp_buf, "\r\n#XDATAMODE: 0\r\n");
		}
		rsp_send(rsp_buf, strlen(rsp_buf));

		slm_operation_mode = SLM_AT_COMMAND_MODE;
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
	err = pm_device_state_set(uart_dev, PM_DEVICE_STATE_OFF, NULL, NULL);
	if (err) {
		LOG_ERR("Can't power off uart: %d", err);
	}

	return err;
}

int poweron_uart(void)
{
	int err;
	uint32_t current_state = 0;

	err = pm_device_state_get(uart_dev, &current_state);
	if (err) {
		LOG_ERR("Device get power_state: %d", err);
		return err;
	}

	if (current_state != PM_DEVICE_STATE_ACTIVE) {
		pm_device_state_set(uart_dev, PM_DEVICE_STATE_ACTIVE, NULL, NULL);
		k_sleep(K_MSEC(100));
		err = uart_receive();
		if (err == 0) {
			k_sem_give(&tx_done);
			rsp_send(SLM_SYNC_STR, sizeof(SLM_SYNC_STR)-1);
		}
	}

	return err;
}

int set_uart_baudrate(uint32_t baudrate)
{
	int err;

	LOG_DBG("Set uart baudrate to: %d", baudrate);

	slm_uart.baudrate = baudrate;
	err = uart_configure(uart_dev, &slm_uart);
	if (err != 0) {
		LOG_ERR("uart_configure: %d", err);
	}

	return err;
}

bool verify_datamode_control(uint16_t time_limit, uint16_t *min_time_limit)
{
	int min_time;

	if (slm_uart.baudrate == 0) {
		LOG_ERR("Baudrate not set");
		return false;
	}

	min_time = UART_RX_LEN * (8 + 1 + 1) * 1000 / slm_uart.baudrate;
	min_time += UART_RX_MARGIN_MS;

	if (time_limit > 0 && min_time > time_limit) {
		LOG_ERR("Invalid time_limit: %d, min: %d", time_limit, min_time);
		return false;
	}

	if (min_time_limit) {
		*min_time_limit = min_time;
	}

	return true;
}

static void response_handler(void *context, const char *response)
{
	int len = strlen(response);

	ARG_UNUSED(context);

	/* Forward the data over UART */
	if (slm_operation_mode == SLM_AT_COMMAND_MODE && len > 0) {
		rsp_send("\r\n", 2);
		rsp_send(response, len);
	}
}

static void raw_send(struct k_work *work)
{
	ARG_UNUSED(work);

	const uint32_t tx_buf_size = sizeof(at_buf) / 2;
	uint32_t size = ring_buf_get(&data_rb, &at_buf[tx_buf_size], tx_buf_size);

	if (size > 0) {
		LOG_INF("Raw send %d", size);
		LOG_HEXDUMP_DBG(&at_buf[tx_buf_size], size, "RX");
		if (datamode_handler) {
			(void)datamode_handler(DATAMODE_SEND, &at_buf[tx_buf_size], size);
		} else {
			LOG_WRN("no handler, data dropped");
		}
	}
	/* resume UART RX in case of stopped by buffer full */
	if (datamode_rx_disabled) {
		uart_receive();
		datamode_rx_disabled = false;
	}
}

static void inactivity_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	LOG_INF("time limit reached");
	if (!ring_buf_is_empty(&data_rb)) {
		k_work_submit(&raw_send_work);
	} else {
		LOG_WRN("data buffer empty");
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

	/* quit datamode */
	if (datamode_handler) {
		(void)datamode_handler(DATAMODE_EXIT, NULL, 0);
		(void)exit_datamode(true);
	} else {
		LOG_WRN("missing datamode handler");
	}
	datamode_off_pending = false;
}

K_TIMER_DEFINE(silence_timer, silence_timer_handler, NULL);

static int raw_rx_handler(const uint8_t *data, int datalen)
{
	int ret;
	const char *quit_str = CONFIG_SLM_DATAMODE_TERMINATOR;
	int quit_str_len = strlen(quit_str);
	int64_t silence = CONFIG_SLM_DATAMODE_SILENCE * MSEC_PER_SEC;
	static int64_t rx_start;

	/* First, check conditions for quitting datamode */
	if (silence > k_uptime_delta(&rx_start)) {
		if (datamode_off_pending) {
			/* quit procedure aborted */
			k_timer_stop(&silence_timer);
			datamode_off_pending = false;
			(void)ring_buf_put(&data_rb, quit_str, quit_str_len);
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

	if (datamode_time_limit > 0) {
		k_timer_stop(&inactivity_timer);
	}

	/* Second, save data to buffer */
	ret = ring_buf_put(&data_rb, data, datalen);
	if (ret != datalen) {
		LOG_ERR("enqueue data error (%d, %d)", datalen, ret);
		uart_rx_disable(uart_dev);
		return -1;
	}
	ret = ring_buf_space_get(&data_rb);
	if (ret < UART_RX_LEN) {
		LOG_WRN("data buffer full (%d)", ret);
		uart_rx_disable(uart_dev);
		return -1;
	}

	/* Third, start/restart inactivity timer, or trigger sending */
	if (datamode_time_limit > 0) {
		k_timer_start(&inactivity_timer, K_MSEC(datamode_time_limit), K_NO_WAIT);
	} else {
		k_work_submit(&raw_send_work);
	}

	rx_start = k_uptime_get();
	return 0;
}

/*
 * Check AT command grammar based on below.
 *  AT<NULL>
 *  AT<separator><body><NULL>
 *  AT<separator><body>=<NULL>
 *  AT<separator><body>?<NULL>
 *  AT<separator><body>=?<NULL>
 *  AT<separator><body>=<parameters><NULL>
 * In which
 * <separator>: +, %, #
 * <body>: alphanumeric char only, size > 0
 * <parameters>: arbitrary, size > 0
 */
static int cmd_grammar_check(const uint8_t *cmd, uint16_t length)
{
	const uint8_t *body;

	/* check AT (if not, no check) */
	if (length < 2 || toupper((int)cmd[0]) != 'A' || toupper((int)cmd[1]) != 'T') {
		return -EINVAL;
	}

	/* check AT<NULL> */
	cmd += 2;
	if (*cmd == '\0') {
		return 0;
	}

	/* check AT<separator> */
	if ((*cmd != '+') && (*cmd != '%') && (*cmd != '#')) {
		return -EINVAL;
	}

	/* check AT<separator><body> */
	cmd += 1;
	body = cmd;
	while (true) {
		/* check body is alphanumeric */
		if (!isalpha((int)*cmd) && !isdigit((int)*cmd)) {
			break;
		}
		cmd++;
	}

	/* check body size > 0 */
	if (cmd == body) {
		return -EINVAL;
	}

	/* check AT<separator><body><NULL> */
	if (*cmd == '\0') {
		return 0;
	}

	/* check AT<separator><body>= or check AT<separator><body>? */
	if (*cmd != '=' && *cmd != '?') {
		return -EINVAL;
	}

	/* check AT<separator><body>?<NULL> */
	if (*cmd == '?') {
		cmd += 1;
		if (*cmd == '\0') {
			return 0;
		} else {
			return -EINVAL;
		}
	}

	/* check AT<separator><body>=<NULL> */
	cmd += 1;
	if (*cmd == '\0') {
		return 0;
	}

	/* check AT<separator><body>=?<NULL> */
	if (*cmd == '?') {
		cmd += 1;
		if (*cmd == '\0') {
			return 0;
		} else {
			return -EINVAL;
		}
	}

	/* no need to check AT<separator><body>=<parameters><NULL> */
	return 0;
}

static void cmd_send(struct k_work *work)
{
	char str[32];
	enum at_cmd_state state;
	int err;

	ARG_UNUSED(work);

	if (at_buf_overflow) {
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}

	LOG_HEXDUMP_DBG(at_buf, at_buf_len, "RX");

	if (cmd_grammar_check(at_buf, at_buf_len) != 0) {
		LOG_ERR("AT command invalid");
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}

	err = slm_at_parse(at_buf);
	if (err == 0) {
		if (!in_datamode()) {
			rsp_send(OK_STR, sizeof(OK_STR) - 1);
		}
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
	(void)uart_receive();
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
		case '\r':
#if defined(CONFIG_SLM_CR_TERMINATION)
			goto send;
#else
			break;
#endif
		case '\n':
#if defined(CONFIG_SLM_LF_TERMINATION)
			goto send;
#elif defined(CONFIG_SLM_CR_LF_TERMINATION)
			if (at_cmd_len > 0 && at_buf[at_cmd_len - 1] == '\r') {
				at_cmd_len--; /* trim the CR char */
				goto send;
			}
#endif
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
		break;
	case UART_TX_ABORTED:
		k_free(uart_tx_buf);
		k_sem_give(&tx_done);
		LOG_INF("TX_ABORTED");
		break;
	case UART_RX_RDY:
		if (slm_operation_mode == SLM_AT_COMMAND_MODE) {
			for (int i = pos; i < (pos + evt->data.rx.len); i++) {
				err = cmd_rx_handler(evt->data.rx.buf[i]);
				if (err) {
					return;
				}
			}
		} else if (slm_operation_mode == SLM_DATA_MODE) {
			err = raw_rx_handler(&(evt->data.rx.buf[pos]), evt->data.rx.len);
			if (err) {
				return;
			}
		} else {
			LOG_WRN("No handler");
		}
		pos += evt->data.rx.len;
		break;
	case UART_RX_BUF_REQUEST:
		pos = 0;
		err = uart_rx_buf_rsp(uart_dev, next_buf, sizeof(uart_rx_buf[0]));
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
		if (slm_operation_mode == SLM_DATA_MODE) {
			datamode_rx_disabled = true;
		}
		if (enable_rx_retry && !uart_recovery_pending) {
			k_work_schedule(&uart_recovery_work, K_MSEC(UART_ERROR_DELAY_MS));
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
	/* Save UART configuration to setting page */
	if (!uart_configured) {
		err = uart_config_get(uart_dev, &slm_uart);
		if (err != 0) {
			LOG_ERR("uart_config_get: %d", err);
			return err;
		}
		err = slm_setting_uart_save();
		if (err != 0) {
			LOG_ERR("uart_config_get: %d", err);
			return err;
		}
		uart_configured = true;
	} /* else re-config UART based on setting page */
	LOG_DBG("UART baud: %d d/p/s-bits: %d/%d/%d HWFC: %d", slm_uart.baudrate,
		slm_uart.data_bits, slm_uart.parity, slm_uart.stop_bits, slm_uart.flow_ctrl);
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
	pm_device_state_set(uart_dev, PM_DEVICE_STATE_ACTIVE, NULL, NULL);
	err = uart_receive();
	if (err) {
		return -EFAULT;
	}

	err = at_notif_register_handler(NULL, response_handler);
	if (err) {
		LOG_ERR("Can't register handler: %d", err);
		return err;
	}

	/* Initialize AT Parser */
	err = at_params_list_init(&at_param_list, CONFIG_SLM_AT_MAX_PARAM);
	if (err) {
		LOG_ERR("Failed to init AT Parser: %d", err);
		return err;
	}

	datamode_time_limit = 0;
	datamode_handler = NULL;

	err = slm_at_init();
	if (err) {
		return -EFAULT;
	}

	k_work_init(&raw_send_work, raw_send);
	k_work_init(&cmd_send_work, cmd_send);
	k_work_init_delayable(&uart_recovery_work, uart_recovery);
	k_sem_give(&tx_done);
	rsp_send(SLM_SYNC_STR, sizeof(SLM_SYNC_STR)-1);
	slm_fota_post_process();

	slm_operation_mode = SLM_AT_COMMAND_MODE;
	LOG_DBG("at_host init done");
	return err;
}

void slm_at_host_uninit(void)
{
	int err;

	if (slm_operation_mode == SLM_DATA_MODE) {
		k_timer_stop(&silence_timer);
		k_timer_stop(&inactivity_timer);
	}
	datamode_handler = NULL;

	slm_at_uninit();

	err = at_notif_deregister_handler(NULL, response_handler);
	if (err) {
		LOG_WRN("Can't deregister handler: %d", err);
	}

	/* Power off UART module */
	uart_rx_disable(uart_dev);
	k_sleep(K_MSEC(100));
	err = pm_device_state_set(uart_dev, PM_DEVICE_STATE_OFF, NULL, NULL);
	if (err) {
		LOG_WRN("Can't power off uart: %d", err);
	}

	LOG_DBG("at_host uninit done");
}
