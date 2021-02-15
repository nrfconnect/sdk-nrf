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
#include <power/reboot.h>

LOG_MODULE_REGISTER(at_host, CONFIG_SLM_LOG_LEVEL);

#include "slm_util.h"
#include "slm_at_host.h"
#include "slm_at_tcp_proxy.h"
#include "slm_at_udp_proxy.h"
#include "slm_at_tcpip.h"
#if defined(CONFIG_SLM_NATIVE_TLS)
#include "slm_at_cmng.h"
#endif
#include "slm_at_icmp.h"
#include "slm_at_fota.h"
#if defined(CONFIG_SLM_GPS)
#include "slm_at_gps.h"
#endif
#if defined(CONFIG_SLM_FTPC)
#include "slm_at_ftp.h"
#endif
#if defined(CONFIG_SLM_MQTTC)
#include "slm_at_mqtt.h"
#endif
#if defined(CONFIG_SLM_HTTPC)
#include "slm_at_httpc.h"
#endif

#define OK_STR		"\r\nOK\r\n"
#define ERROR_STR	"\r\nERROR\r\n"
#define FATAL_STR	"FATAL ERROR\r\n"
#define OVERFLOW_STR	"Buffer overflow\r\n"
#define SLM_SYNC_STR	"Ready\r\n"

#define SLM_VERSION	"#XSLMVER: \"1.5\"\r\n"
#define AT_CMD_SLMVER	"AT#XSLMVER"
#define AT_CMD_SLEEP	"AT#XSLEEP"
#define AT_CMD_RESET	"AT#XRESET"
#define AT_CMD_CLAC	"AT#XCLAC"
#define AT_CMD_SLMUART	"AT#XSLMUART"
#define AT_CMD_DATACTRL	"AT#XDATACTRL"

/** The maximum allowed length of an AT command passed through the SLM
 *  The space is allocated statically. This limit is in turn limited by
 *  Modem library's NRF_MODEM_AT_MAX_CMD_SIZE */
#define AT_MAX_CMD_LEN	4096

#define UART_RX_BUF_NUM	2
#define UART_RX_LEN	256
#define UART_RX_TIMEOUT_MS	1
#define UART_ERROR_DELAY_MS	500
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

/**@brief Shutdown modes. */
enum shutdown_modes {
	SHUTDOWN_MODE_IDLE,
	SHUTDOWN_MODE_SLEEP,
	SHUTDOWN_MODE_INVALID
};

static enum term_modes term_mode;
static const struct device *uart_dev;
static uint8_t at_buf[AT_MAX_CMD_LEN];
static size_t at_buf_len;
static bool at_buf_overflow;
static bool datamode_active;
static bool datamode_off_pending;
static uint16_t datamode_size_limit;
static uint16_t datamode_time_limit;
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
void enter_idle(void);
void enter_sleep(bool wake_up);

/* global variable defined in different files */
extern struct at_param_list at_param_list;
extern char rsp_buf[CONFIG_SLM_SOCKET_RX_MAX * 2];

/* forward declaration */
void slm_at_host_uninit(void);

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

void enter_datamode(void)
{
	datamode_active = true;
	LOG_INF("Enter datamode");
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
		LOG_INF("Exit datamode");
		return true;
	}

	return false;
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

static int set_uart_baudrate(uint32_t baudrate)
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

static int get_uart_baudrate(void)
{
	int err;
	struct uart_config cfg = {
		.baudrate = 0
	};

	err = uart_config_get(uart_dev, &cfg);
	if (err) {
		LOG_ERR("uart_config_get: %d", err);
	}
	return (int)cfg.baudrate;
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

static void handle_at_clac(void)
{
	rsp_send(AT_CMD_SLMVER, sizeof(AT_CMD_SLMVER) - 1);
	rsp_send("\r\n", 2);
	rsp_send(AT_CMD_SLMUART, sizeof(AT_CMD_SLMUART) - 1);
	rsp_send("\r\n", 2);
	rsp_send(AT_CMD_SLEEP, sizeof(AT_CMD_SLEEP) - 1);
	rsp_send("\r\n", 2);
	rsp_send(AT_CMD_RESET, sizeof(AT_CMD_RESET) - 1);
	rsp_send("\r\n", 2);
	rsp_send(AT_CMD_CLAC, sizeof(AT_CMD_CLAC) - 1);
	rsp_send("\r\n", 2);
	rsp_send(AT_CMD_DATACTRL, sizeof(AT_CMD_DATACTRL) - 1);
	rsp_send("\r\n", 2);
	slm_at_tcp_proxy_clac();
	slm_at_udp_proxy_clac();
	slm_at_tcpip_clac();
#if defined(CONFIG_SLM_NATIVE_TLS)
	slm_at_cmng_clac();
#endif
	slm_at_icmp_clac();
	slm_at_fota_clac();
#if defined(CONFIG_SLM_GPS)
	slm_at_gps_clac();
#endif
#if defined(CONFIG_SLM_FTPC)
	slm_at_ftp_clac();
#endif
#if defined(CONFIG_SLM_MQTTC)
	slm_at_mqtt_clac();
#endif
#if defined(CONFIG_SLM_HTTPC)
	slm_at_httpc_clac();
#endif
}

static int handle_at_sleep(const char *at_cmd, enum shutdown_modes *mode)
{
	int ret = -EINVAL;
	enum at_cmd_type type;
	uint16_t shutdown_mode;

	ret = at_parser_params_from_str(at_cmd, NULL, &at_param_list);
	if (ret < 0) {
		LOG_ERR("Failed to parse AT command %d", ret);
		return -EINVAL;
	}

	type = at_parser_cmd_type_get(at_cmd);
	if (type == AT_CMD_TYPE_SET_COMMAND) {
		shutdown_mode = SHUTDOWN_MODE_IDLE;
		if (at_params_valid_count_get(&at_param_list) > 1) {
			ret = at_params_short_get(&at_param_list, 1,
					&shutdown_mode);
			if (ret < 0) {
				LOG_ERR("AT parameter error");
				return -EINVAL;
			}
		}
		if (shutdown_mode == SHUTDOWN_MODE_IDLE) {
			slm_at_host_uninit();
			enter_idle();
			*mode = SHUTDOWN_MODE_IDLE;
			ret = 0; /*Will send no "OK"*/
		} else if (shutdown_mode == SHUTDOWN_MODE_SLEEP) {
			slm_at_host_uninit();
			enter_sleep(true);
			ret = 0; /* Cannot reach here */
		} else {
			LOG_ERR("AT parameter error");
			ret = -EINVAL;
		}
	}

	if (type == AT_CMD_TYPE_TEST_COMMAND) {
		sprintf(rsp_buf, "#XSLEEP: (%d,%d)\r\n", SHUTDOWN_MODE_IDLE,
			SHUTDOWN_MODE_SLEEP);
		rsp_send(rsp_buf, strlen(rsp_buf));
		ret = 0;
	}

	return ret;
}

static int handle_at_slmuart(const char *at_cmd, uint32_t *baudrate)
{
	int ret = -EINVAL;
	enum at_cmd_type type;

	ret = at_parser_params_from_str(at_cmd, NULL, &at_param_list);
	if (ret < 0) {
		LOG_ERR("Failed to parse AT command %d", ret);
		return -EINVAL;
	}

	type = at_parser_cmd_type_get(at_cmd);
	if (type == AT_CMD_TYPE_SET_COMMAND) {
		if (at_params_valid_count_get(&at_param_list) > 1) {
			ret = at_params_int_get(&at_param_list, 1,
					baudrate);
			if (ret < 0) {
				LOG_ERR("AT parameter error");
				return -EINVAL;
			}
		}
		switch (*baudrate) {
		case 1200:
		case 2400:
		case 4800:
		case 9600:
		case 14400:
		case 19200:
		case 38400:
		case 57600:
		case 115200:
		case 230400:
		case 460800:
		case 921600:
		case 1000000:
			ret = 0;
			break;
		default:
			LOG_ERR("Invalid uart baud rate provided.");
			return -EINVAL;
		}
	}

	if (type == AT_CMD_TYPE_READ_COMMAND) {
		sprintf(rsp_buf, "#XSLMUART: %d\r\n", get_uart_baudrate());
		rsp_send(rsp_buf, strlen(rsp_buf));
		ret = 0;
	}

	if (type == AT_CMD_TYPE_TEST_COMMAND) {
		sprintf(rsp_buf, "#XSLMUART: (1200,2400,4800,9600,14400,"
				 "19200,38400,57600,115200,230400,460800,"
				 "921600,1000000)\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		ret = 0;
	}

	return ret;
}

/**@brief handle AT#XDATACTRL commands
 *  AT#XDATACTRL=<size_limit>,<time_limit>
 *  AT#XDATACTRL?
 *  AT#XDATACTRL=?
 */
static int handle_at_datactrl(const char *at_cmd)
{
	int ret;
	uint16_t size_limit;
	uint16_t time_limit;

	ret = at_parser_params_from_str(at_cmd, NULL, &at_param_list);
	if (ret) {
		LOG_ERR("Failed to parse AT command %d", ret);
		return -EINVAL;
	}
	switch (at_parser_cmd_type_get(at_cmd)) {
	case AT_CMD_TYPE_SET_COMMAND:
		ret = at_params_short_get(&at_param_list, 1, &size_limit);
		if (ret) {
			return ret;
		}
		ret = at_params_short_get(&at_param_list, 2, &time_limit);
		if (ret) {
			return ret;
		}
		if (datamode_time_limit < DATAMODE_TIME_LIMIT_MAX &&
		    datamode_size_limit < DATAMODE_SIZE_LIMIT_MAX) {
			datamode_size_limit = size_limit;
			datamode_time_limit = time_limit;
		} else {
			return -EINVAL;
		}
		break;

	case AT_CMD_TYPE_READ_COMMAND:
		sprintf(rsp_buf, "#XDATACTRL: %d,%d\r\n",
				datamode_size_limit, datamode_time_limit);
		rsp_send(rsp_buf, strlen(rsp_buf));
		ret = 0;
		break;

	case AT_CMD_TYPE_TEST_COMMAND:
		sprintf(rsp_buf, "#XDATACTRL=<size_limit>,<time_limit>\r\n");
		rsp_send(rsp_buf, strlen(rsp_buf));
		ret = 0;
		break;

	default:
		break;
	}

	return ret;
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

	if (slm_tcp_get_datamode()) {
		(void)slm_tcp_send_datamode(at_buf, at_buf_len);
		goto sent;
	}

	if (slm_udp_get_datamode()) {
		(void)slm_udp_send_datamode(at_buf, at_buf_len);
		goto sent;
	}

sent:
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
	if (slm_tcp_get_datamode()) {
		slm_tcp_set_datamode_off();
	}
	if (slm_udp_get_datamode()) {
		slm_udp_set_datamode_off();
	}
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

	/* First, check conditions for quiting datamode
	 */
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
		if (datalen == quit_str_len &&
		    strncmp(data, quit_str, quit_str_len) == 0) {
			datamode_off_pending = true;
			/* check subordinate silence */
			k_timer_start(&silence_timer,
			      K_SECONDS(CONFIG_SLM_DATAMODE_SILENCE),
			      K_NO_WAIT);
			LOG_INF("datamode off pending");
			rx_start = k_uptime_get();
			return 0;
		}
	}

	/* Second, check conditions for sending
	 */
	if (datamode_time_limit > 0) {
		k_timer_stop(&inactivity_timer);
	}

	memcpy(at_buf + at_buf_len, data, datalen);
	at_buf_len += datalen;

	if (datamode_size_limit > 0 && at_buf_len >= datamode_size_limit) {
		LOG_INF("size limit reached");
		goto transit;
	}

	/* (re)start inactivity timer */
	if (datamode_time_limit > 0) {
		k_timer_start(&inactivity_timer,
		      K_MSEC(datamode_time_limit),
		      K_NO_WAIT);
	}

	/* Third, check the condition of no size/time limit
	 */
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

	if (slm_util_cmd_casecmp(at_buf, AT_CMD_SLMVER)) {
		rsp_send(SLM_VERSION, sizeof(SLM_VERSION) - 1);
		rsp_send(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	}

	if (slm_util_cmd_casecmp(at_buf, AT_CMD_SLMUART)) {
		uint32_t baudrate;

		err = handle_at_slmuart(at_buf, &baudrate);
		if (err != 0) {
			rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
			goto done;
		} else {
			rsp_send(OK_STR, sizeof(OK_STR) - 1);
			k_sleep(K_MSEC(50));
			set_uart_baudrate(baudrate);
			goto done;
		}
	}

	if (slm_util_cmd_casecmp(at_buf, AT_CMD_RESET)) {
		rsp_send(OK_STR, sizeof(OK_STR) - 1);
		k_sleep(K_MSEC(50));
		slm_at_host_uninit();
		enter_sleep(false);
		sys_reboot(SYS_REBOOT_COLD);
	}

	if (slm_util_cmd_casecmp(at_buf, AT_CMD_CLAC)) {
		handle_at_clac();
		rsp_send(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	}

	if (slm_util_cmd_casecmp(at_buf, AT_CMD_SLEEP)) {
		enum shutdown_modes mode = SHUTDOWN_MODE_INVALID;

		err = handle_at_sleep(at_buf, &mode);
		if (err) {
			rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
			goto done;
		} else {
			if (mode == SHUTDOWN_MODE_INVALID) {
				/*Test command*/
				rsp_send(OK_STR, sizeof(OK_STR) - 1);
				goto done;
			} else {
				/*Entered IDLE*/
				return;
			}
		}
	}

	if (slm_util_cmd_casecmp(at_buf, AT_CMD_DATACTRL)) {
		err = handle_at_datactrl(at_buf);
		if (err == 0) {
			rsp_send(OK_STR, sizeof(OK_STR) - 1);
			goto done;
		} else {
			rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
			goto done;
		}
	}

	err = slm_at_tcp_proxy_parse(at_buf);
	if (err == 0) {
		rsp_send(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	} else if (err != -ENOENT) {
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}

	err = slm_at_udp_proxy_parse(at_buf);
	if (err == 0) {
		rsp_send(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	} else if (err != -ENOENT) {
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}

	err = slm_at_tcpip_parse(at_buf);
	if (err == 0) {
		rsp_send(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	} else if (err != -ENOENT) {
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}

#if defined(CONFIG_SLM_NATIVE_TLS)
	err = slm_at_cmng_parse(at_buf);
	if (err == 0) {
		rsp_send(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	} else if (err != -ENOENT) {
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}
#endif

	err = slm_at_icmp_parse(at_buf);
	if (err == 0) {
		goto done;
	} else if (err != -ENOENT) {
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}

	err = slm_at_fota_parse(at_buf);
	if (err == 0) {
		rsp_send(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	} else if (err != -ENOENT) {
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}

#if defined(CONFIG_SLM_GPS)
	err = slm_at_gps_parse(at_buf);
	if (err == 0) {
		rsp_send(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	} else if (err != -ENOENT) {
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}
#endif

#if defined(CONFIG_SLM_FTPC)
	err = slm_at_ftp_parse(at_buf);
	if (err == 0) {
		rsp_send(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	} else if (err != -ENOENT) {
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}
#endif

#if defined(CONFIG_SLM_MQTTC)
	err = slm_at_mqtt_parse(at_buf);
	if (err == 0) {
		rsp_send(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	} else if (err != -ENOENT) {
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}
#endif

#if defined(CONFIG_SLM_HTTPC)
	err = slm_at_httpc_parse(at_buf, at_buf_len);
	if (err == 0) {
		rsp_send(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	} else if (err != -ENOENT) {
		rsp_send(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}
#endif

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

	datamode_active = false;
	datamode_time_limit = 0;
	datamode_size_limit = 0;

	err = slm_at_tcp_proxy_init();
	if (err) {
		LOG_ERR("TCP Server could not be initialized: %d", err);
		return -EFAULT;
	}
	err = slm_at_udp_proxy_init();
	if (err) {
		LOG_ERR("UDP Server could not be initialized: %d", err);
		return -EFAULT;
	}
	err = slm_at_tcpip_init();
	if (err) {
		LOG_ERR("TCPIP could not be initialized: %d", err);
		return -EFAULT;
	}
#if defined(CONFIG_SLM_NATIVE_TLS)
	err = slm_at_cmng_init();
	if (err) {
		LOG_ERR("TLS could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
	err = slm_at_icmp_init();
	if (err) {
		LOG_ERR("ICMP could not be initialized: %d", err);
		return -EFAULT;
	}
	err = slm_at_fota_init();
	if (err) {
		LOG_ERR("FOTA could not be initialized: %d", err);
		return -EFAULT;
	}
#if defined(CONFIG_SLM_GPS)
	err = slm_at_gps_init();
	if (err) {
		LOG_ERR("GPS could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_FTPC)
	err = slm_at_ftp_init();
	if (err) {
		LOG_ERR("FTP could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_MQTTC)
	err = slm_at_mqtt_init();
	if (err) {
		LOG_ERR("MQTT could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
#if defined(CONFIG_SLM_HTTPC)
	err = slm_at_httpc_init();
	if (err) {
		LOG_ERR("HTTP could not be initialized: %d", err);
		return -EFAULT;
	}
#endif
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

	err = slm_at_tcp_proxy_uninit();
	if (err) {
		LOG_WRN("TCP Server could not be uninitialized: %d", err);
	}
	err = slm_at_udp_proxy_uninit();
	if (err) {
		LOG_WRN("UDP Server could not be uninitialized: %d", err);
	}
	err = slm_at_tcpip_uninit();
	if (err) {
		LOG_WRN("TCPIP could not be uninitialized: %d", err);
	}
#if defined(CONFIG_SLM_NATIVE_TLS)
	err = slm_at_cmng_uninit();
	if (err) {
		LOG_WRN("TLS could not be uninitialized: %d", err);
	}
#endif
	err = slm_at_icmp_uninit();
	if (err) {
		LOG_WRN("ICMP could not be uninitialized: %d", err);
	}
	err = slm_at_fota_uninit();
	if (err) {
		LOG_WRN("FOTA could not be uninitialized: %d", err);
	}
#if defined(CONFIG_SLM_GPS)
	err = slm_at_gps_uninit();
	if (err) {
		LOG_WRN("GPS could not be uninitialized: %d", err);
	}
#endif
#if defined(CONFIG_SLM_FTPC)
	err = slm_at_ftp_uninit();
	if (err) {
		LOG_WRN("FTP could not be uninitialized: %d", err);
	}
#endif
#if defined(CONFIG_SLM_MQTTC)
	err = slm_at_mqtt_uninit();
	if (err) {
		LOG_WRN("MQTT could not be uninitialized: %d", err);
	}
#endif
#if defined(CONFIG_SLM_HTTPC)
	err = slm_at_httpc_uninit();
	if (err) {
		LOG_WRN("HTTP could not be uninitialized: %d", err);
	}
#endif

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
