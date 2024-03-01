/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "slm_at_host.h"
#include "slm_at_fota.h"
#include "slm_uart_handler.h"
#include "slm_util.h"
#if defined(CONFIG_SLM_PPP)
#include "slm_ppp.h"
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/ring_buffer.h>
LOG_MODULE_REGISTER(slm_at_host, CONFIG_SLM_LOG_LEVEL);

#define SLM_SYNC_STR    "Ready\r\n"
#define OK_STR		"\r\nOK\r\n"
#define ERROR_STR	"\r\nERROR\r\n"
#define CRLF_STR	"\r\n"
#define CR		'\r'
#define LF		'\n'
#define HEXDUMP_LIMIT   16

/* Operation mode variables */
enum slm_operation_mode {
	SLM_AT_COMMAND_MODE,		/* AT command host or bridge */
	SLM_DATA_MODE,			/* Raw data sending */
	SLM_NULL_MODE			/* Discard incoming until next command */
};
static struct slm_at_backend at_backend;
static enum slm_operation_mode at_mode;
static slm_datamode_handler_t datamode_handler;
static int datamode_handler_result;
uint16_t slm_datamode_time_limit; /* Send trigger by time in data mode */
K_MUTEX_DEFINE(mutex_mode); /* Protects the operation mode variables. */

static struct at_param_list at_host_param_list;
uint8_t slm_at_buf[SLM_AT_MAX_CMD_LEN + 1];
uint8_t slm_data_buf[SLM_MAX_MESSAGE_SIZE];

RING_BUF_DECLARE(data_rb, CONFIG_SLM_DATAMODE_BUF_SIZE);
static uint8_t quit_str_partial_match;
K_MUTEX_DEFINE(mutex_data); /* Protects the data_rb and quit_str_partial_match. */

static struct k_work raw_send_scheduled_work;

/* global functions defined in different files */
int slm_at_init(void);
void slm_at_uninit(void);

static enum slm_operation_mode get_slm_mode(void)
{
	enum slm_operation_mode mode;

	k_mutex_lock(&mutex_mode, K_FOREVER);
	mode = at_mode;
	k_mutex_unlock(&mutex_mode);

	return mode;
}

/* Lock mutex_mode, before calling. */
static bool set_slm_mode(enum slm_operation_mode mode)
{
	bool ret = false;

	if (at_mode == SLM_AT_COMMAND_MODE) {
		if (mode == SLM_DATA_MODE) {
			ret = true;
		}
	} else if (at_mode == SLM_DATA_MODE) {
		if (mode == SLM_NULL_MODE || mode == SLM_AT_COMMAND_MODE) {
			ret = true;
		}
	} else if (at_mode == SLM_NULL_MODE) {
		if (mode == SLM_AT_COMMAND_MODE || mode == SLM_NULL_MODE) {
			ret = true;
		}
	}

	if (ret) {
		LOG_DBG("SLM mode changed: %d -> %d", at_mode, mode);
		at_mode = mode;
	} else {
		LOG_ERR("Failed to change SLM mode: %d -> %d", at_mode, mode);
	}

	return ret;
}

static bool exit_datamode(void)
{
	bool ret = false;

	k_mutex_lock(&mutex_mode, K_FOREVER);

	if (set_slm_mode(SLM_AT_COMMAND_MODE)) {
		if (datamode_handler) {
			(void)datamode_handler(DATAMODE_EXIT, NULL, 0, SLM_DATAMODE_FLAGS_NONE);
		}
		datamode_handler = NULL;

		k_mutex_lock(&mutex_data, K_FOREVER);
		ring_buf_reset(&data_rb);
		k_mutex_unlock(&mutex_data);

		rsp_send("\r\n#XDATAMODE: %d\r\n", datamode_handler_result);
		datamode_handler_result = 0;

		LOG_INF("Exit datamode");
		ret = true;
	}

	k_mutex_unlock(&mutex_mode);

	return ret;
}

/* Lock mutex_data, before calling. */
static void raw_send(uint8_t flags)
{
	uint8_t *data = NULL;
	int size_send, size_sent, size_all, size_finish;

	/* NOTE ring_buf_get_claim() might not return full size */
	do {
		size_all = ring_buf_size_get(&data_rb);
		size_send = ring_buf_get_claim(&data_rb, &data, CONFIG_SLM_DATAMODE_BUF_SIZE);
		if (size_all != size_send) {
			flags |= SLM_DATAMODE_FLAGS_MORE_DATA;
		}
		LOG_INF("Raw send: size_send: %d, data %p", size_send, (void *)data);
		if (data != NULL && size_send > 0) {

			/* Raw data sending */
			size_finish = 0;

			LOG_HEXDUMP_DBG(data, MIN(size_send, HEXDUMP_LIMIT), "RX");
			k_mutex_lock(&mutex_mode, K_FOREVER);
			if (datamode_handler && size_send > 0) {
				size_sent = datamode_handler(DATAMODE_SEND, data, size_send, flags);
				if (size_sent > 0) {
					size_finish += size_sent;
				} else if (size_sent == 0) {
					size_finish += size_send;
				} else {
					LOG_WRN("Raw send failed, %d dropped", size_send);
					size_finish += size_send;
				}
				(void)ring_buf_get_finish(&data_rb, size_finish);
			} else {
				LOG_WRN("no handler, %d dropped", size_send);
				(void)ring_buf_get_finish(&data_rb, size_send + size_finish);
			}
			k_mutex_unlock(&mutex_mode);

#if defined(CONFIG_SLM_DATAMODE_URC)
			rsp_send("\r\n#XDATAMODE: %d\r\n", size_finish);
#endif
		} else {
			break;
		}
	} while (true);
}

/* Lock mutex_data, before calling. */
static void write_data_buf(const uint8_t *buf, size_t len)
{
	size_t ret;
	size_t index = 0;

	while (index < len) {
		ret = ring_buf_put(&data_rb, buf + index, len - index);
		if (ret) {
			index += ret;
		} else {
			/* Buffer is full. Send data.*/
			raw_send(SLM_DATAMODE_FLAGS_MORE_DATA);
		}
	}
}

static void raw_send_scheduled(struct k_work *work)
{
	ARG_UNUSED(work);

	k_mutex_lock(&mutex_data, K_FOREVER);

	/* Interpret partial quit_str as data, if we send due to timeout. */
	if (quit_str_partial_match > 0) {
		write_data_buf(CONFIG_SLM_DATAMODE_TERMINATOR, quit_str_partial_match);
		quit_str_partial_match = 0;
	}

	raw_send(SLM_DATAMODE_FLAGS_NONE);

	k_mutex_unlock(&mutex_data);
}

static void inactivity_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	LOG_INF("time limit reached");
	if (!ring_buf_is_empty(&data_rb)) {
		k_work_submit(&raw_send_scheduled_work);
	} else {
		LOG_DBG("data buffer empty");
	}
}
K_TIMER_DEFINE(inactivity_timer, inactivity_timer_handler, NULL);

/* Search for quit_str and send data prior to that. Tracks quit_str over several calls. */
static size_t raw_rx_handler(const uint8_t *buf, const size_t len)
{
	const char *const quit_str = CONFIG_SLM_DATAMODE_TERMINATOR;
	size_t processed;
	bool quit_str_match = false;
	bool prev_quit_str_match = false;
	uint8_t quit_str_match_count;
	uint8_t prev_quit_str_match_count;

	k_mutex_lock(&mutex_data, K_FOREVER);

	/* Initialize from previous time.*/
	prev_quit_str_match_count = quit_str_partial_match;
	quit_str_match_count = prev_quit_str_match_count;
	if (prev_quit_str_match_count != 0) {
		prev_quit_str_match = true;
	}

	/* Find quit_str or partial match at the end of the buffer. */
	for (processed = 0; processed < len && quit_str_match == false; processed++) {
		if (buf[processed] == quit_str[quit_str_match_count]) {
			quit_str_match_count++;
			if (quit_str_match_count == strlen(quit_str)) {
				quit_str_match = true;
			}
		} else {
			/* No match. Possible previous partial quit_str match is data. */
			quit_str_match_count = 0;
			prev_quit_str_match = false;
		}
	}

	if (prev_quit_str_match == false) {
		/* Write data which was previously interpreted as a possible partial quit_str. */
		write_data_buf(quit_str, prev_quit_str_match_count);

		/* Write data from buf until the start of the possible (partial) quit_str. */
		write_data_buf(buf, processed - quit_str_match_count);
	} else {
		/* Nothing to write this round.*/
	}

	if (quit_str_match) {
		raw_send(SLM_DATAMODE_FLAGS_NONE);
		(void)exit_datamode();
		quit_str_partial_match = 0;
	} else {
		quit_str_partial_match = quit_str_match_count;
	}

	k_mutex_unlock(&mutex_data);

	return processed;
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
static int cmd_grammar_check(const char *cmd, size_t length)
{
	const char *body;

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

static char *strrstr(const char *str1, const char *str2)
{
	size_t len1;
	size_t len2;

	if (str1 == NULL || str2 == NULL) {
		return NULL;
	}

	len1 = strlen(str1);
	len2 = strlen(str2);

	if (len2 > len1 || len1 == 0 || len2 == 0) {
		return NULL;
	}

	for (int i = len1 - len2; i >= 0; i--) {
		if (strncmp(str1 + i, str2, len2) == 0) {
			return (char *)str1 + i;
		}
	}

	return NULL;
}

static void format_final_result(char *buf, size_t buf_len, size_t buf_max_len)
{
	static const char ok_str[] = "OK\r\n";
	static const char error_str[] = "ERROR\r\n";
	static const char cme_error_str[] = "+CME ERROR:";
	static const char cms_error_str[] = "+CMS ERROR:";
	char *result = NULL;

	result = strrstr(buf, ok_str);
	if (result == NULL) {
		result = strrstr(buf, error_str);
	}

	if (result == NULL) {
		result = strrstr(buf, cme_error_str);
	}

	if (result == NULL) {
		result = strrstr(buf, cms_error_str);
	}

	if (result == NULL) {
		LOG_WRN("Final result not found");
		return;
	}

	/* insert CRLF before final result if there is information response before it */
	if (result != buf + strlen(CRLF_STR)) {
		if (buf_len + strlen(CRLF_STR) < buf_max_len) {
			memmove((void *)(result + strlen(CRLF_STR)), (void *)result,
				strlen(result));
			result[0] = CR;
			result[1] = LF;
			buf_len += strlen(CRLF_STR);
			buf[buf_len] = '\0';
		} else {
			LOG_WRN("No room to insert CRLF");
		}
	}
}
static void restore_at_backend(void)
{
	const int err = at_backend.start();

	if (err) {
		LOG_ERR("Failed to restore AT backend. (%d) Resetting.", err);
		slm_reset();
	}
}

static int stop_at_backend(void)
{
	const int err = at_backend.stop();

	if (!err) {
		/* Wait for UART disabling to complete. */
		k_sleep(K_MSEC(100));
	}
	return err;
}

int slm_at_set_backend(const struct slm_at_backend new_backend)
{
	const struct slm_at_backend old_backend = at_backend;
	int ret;

	if (old_backend.start) {
		ret = stop_at_backend();
		if (ret) {
			LOG_ERR("Failed to stop previous AT backend. (%d)", ret);
			return ret;
		}
	}

	at_backend = new_backend;
	ret = new_backend.start();
	if (ret) {
		LOG_ERR("Failed to start AT backend. (%d)", ret);
		stop_at_backend();

		at_backend = old_backend;
		restore_at_backend();
	}

	return ret;
}

static int slm_at_send_indicate(const uint8_t *data, size_t len,
				bool print_full_debug, bool indicate)
{
	int ret;

	if (k_is_in_isr()) {
		LOG_ERR("FIXME: Attempt to send AT response (of size %u) in ISR.", len);
		return -EINTR;
	} else if (at_backend.send == NULL) {
		LOG_ERR("Attempt to send via an uninitialized AT backend");
		return -EFAULT;
	}

	if (indicate) {
		enum pm_device_state state = PM_DEVICE_STATE_OFF;

		pm_device_state_get(slm_uart_dev, &state);
		if (state != PM_DEVICE_STATE_ACTIVE) {
			slm_indicate();
		}
	}

	ret = at_backend.send(data, len);
	if (!ret) {
		LOG_HEXDUMP_DBG(data, print_full_debug ? len : MIN(HEXDUMP_LIMIT, len), "TX");
	}
	return ret;
}

int slm_at_send(const uint8_t *data, size_t len)
{
	return slm_at_send_indicate(data, len, true, false);
}

int slm_at_send_str(const char *str)
{
	return slm_at_send(str, strlen(str));
}

static void cmd_send(uint8_t *buf, size_t cmd_length, size_t buf_size)
{
	int err;
	size_t offset = 0;
	char *at_cmd = buf;

	LOG_HEXDUMP_DBG(buf, cmd_length, "RX");

	/* UART can send additional characters when the device is powered on.
	 * We ignore everything before the start of the AT-command.
	 */
	while (offset + 1 < cmd_length) {
		if (toupper(buf[offset]) == 'A' && toupper(buf[offset + 1]) == 'T') {
			at_cmd += offset;
			cmd_length -= offset;
			break;
		}
		offset++;
	}

	if (cmd_grammar_check(at_cmd, cmd_length) != 0) {
		LOG_ERR("AT command syntax invalid: %s", at_cmd);
		rsp_send_error();
		return;
	}

	/* Send to modem, reserve space for CRLF in response buffer */
	err = nrf_modem_at_cmd(buf + strlen(CRLF_STR), buf_size - strlen(CRLF_STR), "%s", at_cmd);
	if (err == -SILENT_AT_COMMAND_RET) {
		return;
	} else if (err < 0) {
		LOG_ERR("AT command failed: %d", err);
		rsp_send_error();
		return;
	} else if (err > 0) {
		LOG_ERR("AT command error, type: %d", nrf_modem_at_err_type(err));
	}

	/** Format as TS 27.007 command V1 with verbose response format,
	 *  based on current return of API nrf_modem_at_cmd() and MFWv1.3.x
	 */
	buf[0] = CR;
	buf[1] = LF;
	if (strlen(buf) > strlen(CRLF_STR)) {
		format_final_result(buf, strlen(buf), buf_size);
		err = slm_at_send_str(buf);
		if (err) {
			LOG_ERR("AT command response failed: %d", err);
		}
	}
}

static size_t cmd_rx_handler(const uint8_t *buf, const size_t len)
{
	size_t processed;
	static bool inside_quotes;
	static size_t at_cmd_len;
	static uint8_t prev_character;
	bool send = false;

	for (processed = 0; processed < len && send == false; processed++) {

		/* Handle control characters */
		switch (buf[processed]) {
		case 0x08: /* Backspace. */
			/* Fall through. */
		case 0x7F: /* DEL character */
			if (at_cmd_len > 0) {
				at_cmd_len--;
			}
			continue;
		}

		/* Handle termination characters, if outside quotes. */
		if (!inside_quotes) {
			switch (buf[processed]) {
			case '\r':
				if (IS_ENABLED(CONFIG_SLM_CR_TERMINATION)) {
					send = true;
				}
				break;
			case '\n':
				if (IS_ENABLED(CONFIG_SLM_LF_TERMINATION)) {
					send = true;
				} else if (IS_ENABLED(CONFIG_SLM_CR_LF_TERMINATION)) {
					if (at_cmd_len > 0 && prev_character == '\r') {
						at_cmd_len--; /* trim the CR char */
						send = true;
					}
				}
				break;
			}
		}

		if (send == false) {
			/* Write character to AT buffer, leave space for null */
			if (at_cmd_len < sizeof(slm_at_buf) - 1) {
				slm_at_buf[at_cmd_len] = buf[processed];
			}
			at_cmd_len++;

			/* Handle special written character */
			if (buf[processed] == '"') {
				inside_quotes = !inside_quotes;
			}

			prev_character = buf[processed];
		}
	}

	if (send) {
		if (at_cmd_len > sizeof(slm_at_buf) - 1) {
			LOG_ERR("AT command buffer overflow, %d dropped", at_cmd_len);
			rsp_send_error();
		} else if (at_cmd_len > 0) {
			slm_at_buf[at_cmd_len] = '\0';
			cmd_send(slm_at_buf, at_cmd_len, sizeof(slm_at_buf));
		} else {
			/* Ignore 0 size command. */
		}

		inside_quotes = false;
		at_cmd_len = 0;
	}

	return processed;
}

/* Search for quit_str and exit datamode when one is found. */
static size_t null_handler(const uint8_t *buf, const size_t len)
{
	const char *const quit_str = CONFIG_SLM_DATAMODE_TERMINATOR;
	static size_t dropped_count;
	static uint8_t match_count;

	size_t processed;
	bool match = false;

	if (dropped_count == 0) {
		LOG_WRN("Data pipe broken. Dropping data until datamode is terminated.");
	}

	for (processed = 0; processed < len && match == false; processed++) {
		if (buf[processed] == quit_str[match_count]) {
			match_count++;
			if (match_count == strlen(quit_str)) {
				match = true;
			}
		} else {
			match_count = 0;
		}
		dropped_count++;
	}

	if (match) {
		dropped_count -= strlen(quit_str);
		dropped_count += ring_buf_size_get(&data_rb);
		LOG_WRN("Terminating datamode, %d dropped", dropped_count);
		(void)exit_datamode();

		match_count = 0;
		dropped_count = 0;
	}

	return processed;
}

void slm_at_receive(const uint8_t *buf, size_t len)
{
	size_t ret = 0;

	k_timer_stop(&inactivity_timer);

	while (len > 0) {

		switch (get_slm_mode()) {
		case SLM_AT_COMMAND_MODE:
			ret = cmd_rx_handler(buf, len);
			break;
		case SLM_DATA_MODE:
			ret = raw_rx_handler(buf, len);
			break;
		case SLM_NULL_MODE:
			ret = null_handler(buf, len);
			break;
		}

		assert(ret <= len);
		buf += ret;
		len -= ret;
	}

	/* start inactivity timer in datamode */
	if (get_slm_mode() == SLM_DATA_MODE) {
		k_timer_start(&inactivity_timer, K_MSEC(slm_datamode_time_limit), K_NO_WAIT);
	}
}

AT_MONITOR(at_notify, ANY, notification_handler);

static void notification_handler(const char *notification)
{
	if (get_slm_mode() == SLM_AT_COMMAND_MODE) {

#if defined(CONFIG_SLM_PPP)
		if (!slm_fwd_cgev_notifs
		 && !strncmp(notification, "+CGEV: ", strlen("+CGEV: "))) {
			/* CGEV notifications are silenced. Do not forward them. */
			return;
		}
#endif
		slm_at_send_indicate(CRLF_STR, strlen(CRLF_STR), true, true);
		slm_at_send_str(notification);
	}
}

void rsp_send_ok(void)
{
	slm_at_send_str(OK_STR);
}

void rsp_send_error(void)
{
	slm_at_send_str(ERROR_STR);
}

void rsp_send(const char *fmt, ...)
{
	static K_MUTEX_DEFINE(mutex_rsp_buf);
	static char rsp_buf[SLM_AT_MAX_RSP_LEN];
	int rsp_len;

	k_mutex_lock(&mutex_rsp_buf, K_FOREVER);

	va_list arg_ptr;

	va_start(arg_ptr, fmt);
	rsp_len = vsnprintf(rsp_buf, sizeof(rsp_buf), fmt, arg_ptr);
	rsp_len = MIN(rsp_len, sizeof(rsp_buf) - 1);
	va_end(arg_ptr);

	slm_at_send(rsp_buf, rsp_len);

	k_mutex_unlock(&mutex_rsp_buf);
}

void data_send(const uint8_t *data, size_t len)
{
	slm_at_send_indicate(data, len, false, true);
}

int enter_datamode(slm_datamode_handler_t handler)
{
	k_mutex_lock(&mutex_mode, K_FOREVER);

	if (handler == NULL || datamode_handler != NULL || set_slm_mode(SLM_DATA_MODE) == false) {
		LOG_INF("Invalid, not enter datamode");
		k_mutex_unlock(&mutex_mode);
		return -EINVAL;
	}

	k_mutex_lock(&mutex_data, K_FOREVER);
	ring_buf_reset(&data_rb);
	k_mutex_unlock(&mutex_data);

	datamode_handler = handler;
	if (slm_datamode_time_limit == 0) {
		if (slm_uart_baudrate > 0) {
			slm_datamode_time_limit = CONFIG_SLM_UART_RX_BUF_SIZE * (8 + 1 + 1) * 1000 /
					      slm_uart_baudrate;
			slm_datamode_time_limit += UART_RX_MARGIN_MS;
		} else {
			LOG_WRN("Baudrate not set");
			slm_datamode_time_limit = 1000;
		}
	}
	LOG_INF("Enter datamode");

	k_mutex_unlock(&mutex_mode);

	return 0;
}

bool in_datamode(void)
{
	return (get_slm_mode() == SLM_DATA_MODE);
}

bool exit_datamode_handler(int result)
{
	bool ret = false;

	k_mutex_lock(&mutex_mode, K_FOREVER);

	if (set_slm_mode(SLM_NULL_MODE)) {
		if (datamode_handler) {
			datamode_handler(DATAMODE_EXIT, NULL, 0, SLM_DATAMODE_FLAGS_EXIT_HANDLER);
		}
		datamode_handler = NULL;
		datamode_handler_result = result;
		ret = true;
	}

	k_mutex_unlock(&mutex_mode);

	return ret;
}

bool verify_datamode_control(uint16_t time_limit, uint16_t *min_time_limit)
{
	int min_time;

	if (slm_uart_baudrate == 0) {
		LOG_ERR("Baudrate not set");
		return false;
	}

	min_time = CONFIG_SLM_UART_RX_BUF_SIZE * (8 + 1 + 1) * 1000 / slm_uart_baudrate;
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

int slm_get_at_param_list(const char *at_cmd, struct at_param_list **list)
{
	int err;

	*list = &at_host_param_list;

	err = at_parser_params_from_str(at_cmd, NULL, *list);
	if (err) {
		LOG_ERR("AT command parsing failed: %d", err);
	}

	return err;
}

int slm_at_cb_wrapper(char *buf, size_t len, char *at_cmd, slm_at_callback *cb)
{
	int err;
	struct at_param_list *list = NULL;

	assert(cb);

	err = slm_get_at_param_list(at_cmd, &list);
	if (err) {
		return err;
	}
	err = cb(at_parser_cmd_type_get(at_cmd), list, at_params_valid_count_get(list));
	if (!err) {
		err = at_cmd_custom_respond(buf, len, "OK\r\n");
		if (err) {
			LOG_ERR("Failed to set OK response: %d", err);
		}
	}

	return err;
}

int slm_at_host_init(void)
{
	int err;

	/* Initialize AT Parser */
	err = at_params_list_init(&at_host_param_list, CONFIG_SLM_AT_MAX_PARAM);
	if (err) {
		LOG_ERR("Failed to init AT Parser: %d", err);
		return err;
	}

	k_mutex_lock(&mutex_mode, K_FOREVER);
	slm_datamode_time_limit = 0;
	datamode_handler = NULL;
	at_mode = SLM_AT_COMMAND_MODE;
	k_mutex_unlock(&mutex_mode);

	err = slm_at_init();
	if (err) {
		return -EFAULT;
	}

	k_work_init(&raw_send_scheduled_work, raw_send_scheduled);

	err = slm_uart_handler_enable();
	if (err) {
		return err;
	}

	err = slm_at_send_str(SLM_SYNC_STR);
	if (err) {
		return err;
	}

	/* This is here and not earlier because in case of firmware
	 * update it will send an AT response so the UART must be up.
	 */
	slm_fota_post_process();

	LOG_INF("at_host init done");
	return 0;
}

static int at_host_power_off(bool shutting_down)
{
	int err = stop_at_backend();

	if (!err || shutting_down) {

		/* Power off UART module */
		err = pm_device_action_run(slm_uart_dev, PM_DEVICE_ACTION_SUSPEND);
		if (err == -EALREADY) {
			err = 0;
		}
		if (err) {
			LOG_WRN("Failed to suspend UART. (%d)", err);
			if (!shutting_down) {
				restore_at_backend();
			}
		}
	}

	return err;
}

int slm_at_host_power_off(void)
{
	const int err = at_host_power_off(false);

	/* Write sync str to buffer so it is sent first when resuming. */
	slm_at_send_str(SLM_SYNC_STR);

	return err;
}

int slm_at_host_power_on(void)
{
	const int err = pm_device_action_run(slm_uart_dev, PM_DEVICE_ACTION_RESUME);

	if (err && err != -EALREADY) {
		LOG_ERR("Failed to resume UART. (%d)", err);
		return err;
	}

	/* Wait for UART enabling to complete. */
	k_sleep(K_MSEC(100));

	restore_at_backend();
	return 0;
}

void slm_at_host_uninit(void)
{
	k_mutex_lock(&mutex_mode, K_FOREVER);
	if (at_mode == SLM_DATA_MODE) {
		k_timer_stop(&inactivity_timer);
	}
	datamode_handler = NULL;
	k_mutex_unlock(&mutex_mode);

	slm_at_uninit();

	at_host_power_off(true);

	/* Un-initialize AT Parser */
	at_params_list_free(&at_host_param_list);

	LOG_DBG("at_host uninit done");
}
