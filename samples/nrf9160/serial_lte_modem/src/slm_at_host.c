/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <stdio.h>
#include <ctype.h>
#include <logging/log.h>
#include <drivers/uart.h>
#include <string.h>
#include <init.h>
#include <at_cmd.h>
#include <at_notif.h>

LOG_MODULE_REGISTER(at_host, CONFIG_SLM_LOG_LEVEL);

#include "slm_at_host.h"
#include "slm_at_tcpip.h"
#include "slm_at_icmp.h"
#include "slm_at_gps.h"

#define SLM_UART_0_NAME      "UART_0"
#define SLM_UART_2_NAME      "UART_2"

#define OK_STR    "OK\r\n"
#define ERROR_STR "ERROR\r\n"

#define SLM_SYNC_STR "Ready\r\n"

#define SLM_VERSION	"#XSLMVER: 1.2\r\n"
#define AT_CMD_SLMVER	"AT#XSLMVER"
#define AT_CMD_SLEEP	"AT#XSLEEP"
#define AT_CMD_CLAC	"AT#XCLAC"

#define AT_MAX_CMD_LEN		CONFIG_AT_CMD_RESPONSE_MAX_LEN

/** @brief Termination Modes. */
enum term_modes {
	MODE_NULL_TERM, /**< Null Termination */
	MODE_CR,        /**< CR Termination */
	MODE_LF,        /**< LF Termination */
	MODE_CR_LF,     /**< CR+LF Termination */
	MODE_COUNT      /* Counter of term_modes */
};


/** @brief UARTs. */
enum select_uart {
	UART_0,
	UART_2
};

static enum term_modes term_mode;
static struct device *uart_dev;
static u8_t at_buf[AT_MAX_CMD_LEN];
static size_t at_buf_len;
static struct k_work cmd_send_work;
static const char termination[3] = { '\0', '\r', '\n' };

/* global variable defined in different files */
extern struct at_param_list m_param_list;
extern void enter_sleep(void);

/* forward declaration */
void slm_at_host_uninit(void);

static inline void write_uart_string(const char *str, size_t len)
{
	LOG_HEXDUMP_DBG(str, len, "TX");

	for (size_t i = 0; i < len; i++) {
		uart_poll_out(uart_dev, str[i]);
	}
}

static void response_handler(void *context, const char *response)
{
	int len = strlen(response) + 1;

	ARG_UNUSED(context);

	/* Forward the data over UART */
	if (len > 1) {
		write_uart_string(response, len);
	}
}

static void handle_at_clac(void)
{
	write_uart_string(AT_CMD_SLMVER, sizeof(AT_CMD_SLMVER) - 1);
	write_uart_string("\r\n", 2);
	write_uart_string(AT_CMD_SLEEP, sizeof(AT_CMD_SLEEP) - 1);
	write_uart_string("\r\n", 2);
	write_uart_string(AT_CMD_CLAC, sizeof(AT_CMD_CLAC) - 1);
	write_uart_string("\r\n", 2);
	slm_at_tcpip_clac();
	slm_at_icmp_clac();
	slm_at_gps_clac();
}

static int handle_at_sleep(const char *at_cmd)
{
	int ret = -EINVAL;
	enum at_cmd_type type;

	ret = at_parser_params_from_str(at_cmd, NULL, &m_param_list);
	if (ret < 0) {
		LOG_ERR("Failed to parse AT command %d", ret);
		return -EINVAL;
	}

	type = at_parser_cmd_type_get(at_cmd);
	if (type == AT_CMD_TYPE_SET_COMMAND) {
		slm_at_host_uninit();
		enter_sleep();
		return 0; /* Cannot reach here */
	}

	return ret;
}

static void cmd_send(struct k_work *work)
{
	size_t chars;
	char str[15];
	static char buf[AT_MAX_CMD_LEN];
	enum at_cmd_state state;
	int err;
	size_t size_cmd = sizeof(AT_CMD_SLMVER) - 1;

	ARG_UNUSED(work);

	/* Make sure the string is 0-terminated */
	at_buf[MIN(at_buf_len, AT_MAX_CMD_LEN - 1)] = 0;

	LOG_HEXDUMP_DBG(at_buf, at_buf_len, "RX");

	if (slm_at_cmd_cmp(at_buf, AT_CMD_SLMVER, size_cmd)) {
		write_uart_string(SLM_VERSION, sizeof(SLM_VERSION) - 1);
		write_uart_string(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	}

	size_cmd = sizeof(AT_CMD_CLAC) - 1;
	if (slm_at_cmd_cmp(at_buf, AT_CMD_CLAC, size_cmd)) {
		handle_at_clac();
		write_uart_string(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	}

	size_cmd = sizeof(AT_CMD_SLEEP) - 1;
	if (slm_at_cmd_cmp(at_buf, AT_CMD_SLEEP, size_cmd)) {
		err = handle_at_sleep(at_buf);
		if (err != 0) {
			write_uart_string(ERROR_STR, sizeof(ERROR_STR) - 1);
			goto done;
		} else {
			return;
		}
	}

	err = slm_at_tcpip_parse(at_buf);
	if (err == 0) {
		write_uart_string(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	} else if (err != -ENOTSUP) {
		write_uart_string(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}

	err = slm_at_icmp_parse(at_buf);
	if (err == 0) {
		goto done;
	} else if (err != -ENOTSUP) {
		write_uart_string(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}

	err = slm_at_gps_parse(at_buf);
	if (err == 0) {
		write_uart_string(OK_STR, sizeof(OK_STR) - 1);
		goto done;
	} else if (err != -ENOTSUP) {
		write_uart_string(ERROR_STR, sizeof(ERROR_STR) - 1);
		goto done;
	}

	err = at_cmd_write(at_buf, buf, AT_MAX_CMD_LEN, &state);
	if (err < 0) {
		LOG_ERR("Could not send AT command to modem: %d", err);
		state = AT_CMD_ERROR;
	}

	switch (state) {
	case AT_CMD_OK:
		write_uart_string(buf, strlen(buf));
		write_uart_string(OK_STR, sizeof(OK_STR) - 1);
		break;
	case AT_CMD_ERROR:
		write_uart_string(ERROR_STR, sizeof(ERROR_STR) - 1);
		break;
	case AT_CMD_ERROR_CMS:
		chars = sprintf(str, "+CMS: %d\r\n", err);
		write_uart_string(str, ++chars);
		break;
	case AT_CMD_ERROR_CME:
		chars = sprintf(str, "+CME: %d\r\n", err);
		write_uart_string(str, ++chars);
		break;
	default:
		break;
	}

done:
	uart_irq_rx_enable(uart_dev);
}

static void uart_rx_handler(u8_t character)
{
	static bool inside_quotes;
	static size_t cmd_len;
	size_t pos;

	cmd_len += 1;
	pos = cmd_len - 1;

	/* Handle special characters. */
	switch (character) {
	case 0x08: /* Backspace. */
		/* Fall through. */
	case 0x7F: /* DEL character */
		pos = pos ? pos - 1 : 0;
		at_buf[pos] = 0;
		cmd_len = cmd_len <= 1 ? 0 : cmd_len - 2;
		break;
	case '"':
		inside_quotes = !inside_quotes;
		 /* Fall through. */
	default:
		/* Detect AT command buffer overflow or zero length */
		if (cmd_len > AT_MAX_CMD_LEN) {
			LOG_ERR("Buffer overflow, dropping '%c'\n", character);
			cmd_len = AT_MAX_CMD_LEN;
			return;
		} else if (cmd_len < 1) {
			LOG_ERR("Invalid AT command length: %d", cmd_len);
			cmd_len = 0;
			return;
		}

		at_buf[pos] = character;
		break;
	}

	if (inside_quotes) {
		return;
	}

	/* Check if the character marks line termination. */
	switch (term_mode) {
	case MODE_NULL_TERM:
		/* Fall through. */
	case MODE_CR:
		if (character == termination[term_mode]) {
			goto send;
		}
		break;
	case MODE_LF:
		if ((at_buf[pos - 1]) &&
			character == termination[term_mode]) {
			goto send;
		}
		break;
	case MODE_CR_LF:
		if ((at_buf[pos - 1] == '\r') && (character == '\n')) {
			goto send;
		}
		break;
	default:
		LOG_ERR("Invalid termination mode: %d", term_mode);
		break;
	}

	return;
send:
	uart_irq_rx_disable(uart_dev);
	k_work_submit(&cmd_send_work);
	at_buf_len = cmd_len;
	cmd_len = 0;
}

static void isr(struct device *dev)
{
	u8_t character;

	uart_irq_update(dev);

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	while (uart_fifo_read(dev, &character, 1)) {
		uart_rx_handler(character);
	}
}

static int at_uart_init(char *uart_dev_name)
{
	int err;
	u8_t dummy;

	uart_dev = device_get_binding(uart_dev_name);
	if (uart_dev == NULL) {
		LOG_ERR("Cannot bind %s\n", uart_dev_name);
		return -EINVAL;
	}
	/* Wait for the UART line to become valid */
	do {
		err = uart_err_check(uart_dev);
		if (err) {
			LOG_ERR("UART check failed: %d. "
				"Dropping buffer and retrying.", err);

			while (uart_fifo_read(uart_dev, &dummy, 1)) {
				/* Do nothing with the data */
			}
			k_sleep(10);
		}
	} while (err);

	uart_irq_callback_set(uart_dev, isr);
	return err;
}

static void slm_at_callback(const char *str)
{
	write_uart_string(str, strlen(str));
}

int slm_at_host_init(void)
{
	char *uart_dev_name;
	int err;
	enum select_uart uart_id = CONFIG_SLM_CONNECT_UART;
	enum term_modes mode = CONFIG_SLM_AT_HOST_TERMINATION;

	/* Choosing the termination mode */
	if (mode < MODE_COUNT) {
		term_mode = mode;
	} else {
		return -EINVAL;
	}

	/* Choose which UART to use */
	switch (uart_id) {
	case UART_0:
		uart_dev_name = SLM_UART_0_NAME;
		break;
	case UART_2:
		uart_dev_name = SLM_UART_2_NAME;
		break;
	default:
		LOG_ERR("Unknown UART instance %d", uart_id);
		return -EINVAL;
	}

	err = at_notif_register_handler(NULL, response_handler);
	if (err != 0) {
		LOG_ERR("Can't register handler err=%d", err);
		return err;
	}

	/* Initialize the UART module */
	err = at_uart_init(uart_dev_name);
	if (err) {
		LOG_ERR("UART can not be initialized: %d", err);
		return -EFAULT;
	}

	k_work_init(&cmd_send_work, cmd_send);
	uart_irq_rx_enable(uart_dev);

	write_uart_string(SLM_SYNC_STR, sizeof(SLM_SYNC_STR)-1);

	err = slm_at_tcpip_init(slm_at_callback);
	if (err) {
		LOG_ERR("TCPIP could not be initialized: %d", err);
		return -EFAULT;
	}
	err = slm_at_icmp_init(slm_at_callback);
	if (err) {
		LOG_ERR("ICMP could not be initialized: %d", err);
		return -EFAULT;
	}
	err = slm_at_gps_init(slm_at_callback);
	if (err) {
		LOG_ERR("GPS could not be initialized: %d", err);
		return -EFAULT;
	}

	LOG_DBG("at_host init done");
	return err;
}

void slm_at_host_uninit(void)
{
	int err;

	err = slm_at_tcpip_uninit();
	if (err) {
		LOG_WRN("TCPIP could not be uninitialized: %d", err);
	}
	err = slm_at_icmp_uninit();
	if (err) {
		LOG_WRN("ICMP could not be uninitialized: %d", err);
	}
	err = slm_at_gps_uninit();
	if (err) {
		LOG_WRN("GPS could not be uninitialized: %d", err);
	}
	err = at_notif_deregister_handler(NULL, response_handler);
	if (err != 0) {
		LOG_WRN("Can't deregister handler err=%d", err);
	}
#if defined(CONFIG_DEVICE_POWER_MANAGEMENT)
	err = device_set_power_state(uart_dev, DEVICE_PM_OFF_STATE,
				NULL, NULL);
	if (err != 0) {
		LOG_WRN("Can't power off uart err=%d", err);
	}
#endif
	LOG_DBG("at_host uninit done");
}
