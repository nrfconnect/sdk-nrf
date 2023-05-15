/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <nrf_errno.h>
#include <nrf_modem_at.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <modem/at_cmd_custom.h>

/* Even though it's defined in minimal libc, the declaration of strtok_r appears to be missing in
 * some cases, so we forward declare it here.
 */
extern char *strtok_r(char *, const char *, char **);

LOG_MODULE_REGISTER(gcf_sms, CONFIG_GCF_SMS_LOG_LEVEL);

/* Buffer size for SMS messages. */
#define SMS_BUFFER_ARRAY_SIZE (3)
#define SMS_BUFFER_PDU_SIZE_MAX (352)

/* Max size of Service Center Address. */
#define SCA_SIZE_MAX (16)

/* Buffer for SMS messages. */
struct sms_buffer {
	size_t pdu_size;
	char data[SMS_BUFFER_PDU_SIZE_MAX];
};

/* Buffer list for SMS messages. */
static struct sms_buffer sms_buffers[SMS_BUFFER_ARRAY_SIZE];

/* Buffer for SCA. */
static char sca_buff[SCA_SIZE_MAX + 1];

/* AT filters
 * Including all commands the filter should check for and the respective
 * functions to be called on detection.
 */
AT_CMD_CUSTOM(CPMS, "AT+CPMS", gcf_sms_filter_callback);
AT_CMD_CUSTOM(CSMS, "AT+CSMS", gcf_sms_filter_callback);
AT_CMD_CUSTOM(CSCA, "AT+CSCA", gcf_sms_filter_callback);
AT_CMD_CUSTOM(CSCS, "AT+CSCS", gcf_sms_filter_callback);
AT_CMD_CUSTOM(CMGD, "AT+CMGD", gcf_sms_filter_callback);
AT_CMD_CUSTOM(CMSS, "AT+CMSS", gcf_sms_filter_callback);
AT_CMD_CUSTOM(CMGW, "AT+CMGW", gcf_sms_filter_callback);
AT_CMD_CUSTOM(CMMS, "AT+CMMS", gcf_sms_filter_callback);
/* Be extra strict on AT+CMGF as implementation for other parameters may be in modem. */
AT_CMD_CUSTOM(CMGF0, "AT+CMGF=0", gcf_sms_filter_callback);

/*
 * Internal GCF SMS callbacks
 *
 * note: We use internal callbacks and a common AT custom command callback to support
 * multiple commands in the same call, e.g. "AT+CMMS=1;+CMSS=0;+CMSS=1".
 */

/* Custom AT command callback declarations. */
static int at_cmd_callback_cpms(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_csms(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_csca(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cscs(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cmgd(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cmss(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cmms(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cmgw(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cmgf(char *buf, size_t len, char *at_cmd);

struct nrf_modem_at_cmd_custom gcf_sms_filters[] = {
	{ "AT+CPMS", at_cmd_callback_cpms },
	{ "AT+CSMS", at_cmd_callback_csms },
	{ "AT+CSCA", at_cmd_callback_csca },
	{ "AT+CSCS", at_cmd_callback_cscs },
	{ "AT+CMGD", at_cmd_callback_cmgd },
	{ "AT+CMSS", at_cmd_callback_cmss },
	{ "AT+CMMS", at_cmd_callback_cmms },
	{ "AT+CMGW", at_cmd_callback_cmgw },
	{ "AT+CMGF", at_cmd_callback_cmgf },
};

#define NUM_FILTERS_INTERNAL (sizeof(gcf_sms_filters) / sizeof(struct nrf_modem_at_cmd_custom))

/*  Check if prefix is identical to start of string. */
static bool prefix_match(char *pre, const char *str)
{
	if (strlen(pre) > strlen(str)) {
		return false;
	}

	return (strncmp(pre, str, strlen(pre)) == 0);
}

/* Clear SMS buffer. */
static void sms_buffers_clear_index(int index)
{
	sms_buffers[index].pdu_size = 0;
}

/* Get number of SMS buffers in use. */
static int sms_buffers_num_used_get(void)
{
	int sms_buffers_used = 0;
	/* Find number of storage in use. */
	for (int i = 0; i < SMS_BUFFER_ARRAY_SIZE; i++) {
		if (sms_buffers[i].pdu_size != 0) {
			sms_buffers_used++;
		}
	}
	return sms_buffers_used;
}

/* Get internal callback. */
static nrf_modem_at_cmd_custom_handler_t filter_cb_get(char *at_cmd)
{
	bool match;

	for (size_t i = 0; i < NUM_FILTERS_INTERNAL; i++) {
		match = strstr(at_cmd, gcf_sms_filters[i].cmd);
		if (match) {
			return gcf_sms_filters[i].callback;
		}
	}

	return NULL;
}

/* Callback for filter matches in the Modem library.
 * Implements the internal filter with multiple command support.
 */
int gcf_sms_filter_callback(char *buf, size_t len, char *at_cmd)
{
	int err = 0;
	nrf_modem_at_cmd_custom_handler_t callback;
	char *msg, *msg_rest;
	char *buf_remaining = buf;
	size_t len_remaining = len;
	int len_tmp = 0;
	int cmss_cmgs_commands_count = 0;
	int num_cmss_cmgs_commands = 0;

	if (!at_cmd) {
		return -NRF_EFAULT;
	}

	/* Count number of CMSS' and GMGS' in the concatenated AT command. */
	const char *tmp = at_cmd;

	while ((tmp = strstr(tmp, "+CMSS="))) {
		num_cmss_cmgs_commands++;
		tmp++;
	}

	tmp = at_cmd;
	while ((tmp = strstr(tmp, "+CMGS="))) {
		num_cmss_cmgs_commands++;
		tmp++;
	}

	/* Split and loop on multiple commands. */
	msg = strtok_r(at_cmd, ";", &msg_rest);

	if (!msg) {
		err = -NRF_EFAULT;
		return err;
	}

	while (msg) {
		/* Count CMSS and CMGS commands and send CMMS=0 before the last. */
		if (prefix_match("AT+CMSS=", msg) || prefix_match("AT+CMGS=", msg)) {
			cmss_cmgs_commands_count++;

			if (cmss_cmgs_commands_count == num_cmss_cmgs_commands) {
				LOG_DBG("Sending AT+CMMS=0");
				nrf_modem_at_printf("AT+CMMS=0");
			}
		}

		callback = filter_cb_get(msg);

		if (callback != NULL) {
			/* AT command is filtered. */
			err = (*callback)(buf_remaining, len_remaining, msg);
		} else {
			err = nrf_modem_at_cmd(buf_remaining, len_remaining, msg);
		}

		if (err != 0) {
			return err;
		}

		/* Get the next command. */
		msg = strtok_r(msg_rest, ";", &msg_rest);
		/* Add AT for concatenated AT-commands. */
		if ((msg) && (msg - 2 * sizeof(char) >= at_cmd)) {
			msg = msg - 2 * sizeof(char);
			msg[0] = 'A';
			msg[1] = 'T';
		}

		/* Move head of response buffer after current reply. */
		len_tmp = strlen(buf_remaining);
		buf_remaining = &(buf_remaining[len_tmp]);
		len_remaining = len_remaining - len_tmp;
	}

	return err;
}

/*
 * Custom AT command callback functions.
 */

static int at_cmd_callback_cpms(char *buf, size_t len, char *at_cmd)
{
	int sms_buffers_used = sms_buffers_num_used_get();

	/* Test */
	if (prefix_match("AT+CPMS=?", at_cmd)) {
		return at_cmd_custom_respond(buf, len, "+CPMS: (\"TA\"),(\"TA\")\r\nOK\r\n");
	}

	/* Set */
	if (prefix_match("AT+CPMS=\"TA\",\"TA\"", at_cmd)) {
		return at_cmd_custom_respond(buf, len,
					     "+CPMS: %d,%d,%d,%d\r\nOK\r\n",
					     sms_buffers_used, SMS_BUFFER_ARRAY_SIZE,
					     sms_buffers_used, SMS_BUFFER_ARRAY_SIZE);
	}
	if (prefix_match("AT+CPMS=\"SM\",\"SM\",\"MT\"", at_cmd)) {
		return at_cmd_custom_respond(buf, len,
					     "+CPMS: \"SM\",%d,%d,"
					     "\"SM\",%d,%d,\"MT\",%d,%d\r\nOK\r\n",
					     sms_buffers_used, SMS_BUFFER_ARRAY_SIZE,
					     sms_buffers_used, SMS_BUFFER_ARRAY_SIZE,
					     sms_buffers_used, SMS_BUFFER_ARRAY_SIZE);
	}

	/* Read */
	if (prefix_match("AT+CPMS?", at_cmd)) {
		return at_cmd_custom_respond(buf, len,
					     "+CPMS: \"TA\",%d,%d,\"TA\",%d,%d\r\nOK\r\n",
					     sms_buffers_used, SMS_BUFFER_ARRAY_SIZE,
					     sms_buffers_used, SMS_BUFFER_ARRAY_SIZE);
	}

	return at_cmd_custom_respond(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_csms(char *buf, size_t len, char *at_cmd)
{
	/* Test */
	if (prefix_match("AT+CSMS=?", at_cmd)) {
		return at_cmd_custom_respond(buf, len, "+CSMS: 0\r\nOK\r\n");
	}

	/* Set */
	if (prefix_match("AT+CSMS=0", at_cmd)) {
		return at_cmd_custom_respond(buf, len, "+CSMS: 1,1,0\r\nOK\r\n");
	} else if (prefix_match("AT+CSMS=1", at_cmd)) {
		return at_cmd_custom_respond(buf, len, "+CSMS: 0,0,0\r\nOK\r\n");
	}

	/* Read */
	if (prefix_match("AT+CSMS?", at_cmd)) {
		return at_cmd_custom_respond(buf, len, "+CSMS: 0,1,1,0\r\nOK\r\n");
	}

	return at_cmd_custom_respond(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_csca(char *buf, size_t len, char *at_cmd)
{
	/* Set */
	if (prefix_match("AT+CSCA=", at_cmd)) {
		strncpy(sca_buff, &(at_cmd[8]), SCA_SIZE_MAX);
		sca_buff[sizeof(sca_buff) - 1] = '\0';
		return at_cmd_custom_respond(buf, len, "OK\r\n");
	}

	/* Read */
	if (prefix_match("AT+CSCA?", at_cmd)) {
		return at_cmd_custom_respond(buf, len, "+CSMS: %s\r\nOK\r\n", sca_buff);
	}

	return at_cmd_custom_respond(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_cscs(char *buf, size_t len, char *at_cmd)
{
	/* Set */
	if (prefix_match("AT+CSCS=", at_cmd)) {
		return at_cmd_custom_respond(buf, len, "OK\r\n");
	}

	return at_cmd_custom_respond(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_cmgd(char *buf, size_t len, char *at_cmd)
{
	char *s1 = NULL;
	int index;
	int flag;

	/* Test */
	if (prefix_match("AT+CMGD=?", at_cmd)) {
		return at_cmd_custom_respond(
			buf, len, "+CMGD: (0-%d)\r\nOK\r\n", SMS_BUFFER_ARRAY_SIZE - 1);
	}

	/* Set */
	if (prefix_match("AT+CMGD=", at_cmd)) {
		/* Check flags first. */
		s1 = strstr(at_cmd, ",");
		if (s1) {
			flag = strtol(&(s1[1]), NULL, 10);
			if (flag == 4) {
				/* Delete all buffers. */
				for (int i = 0; i < SMS_BUFFER_ARRAY_SIZE; i++) {
					sms_buffers_clear_index(i);
				}
				return at_cmd_custom_respond(buf, len, "OK\r\n");
			}
		}

		/* No supported flag, delete only the buffer. */
		index = strtol(&(at_cmd[8]), NULL, 10);

		if (index >= 0 && index < SMS_BUFFER_ARRAY_SIZE) {
			sms_buffers_clear_index(index);
			return at_cmd_custom_respond(buf, len, "OK\r\n");
		}
	}

	return at_cmd_custom_respond(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_cmss(char *buf, size_t len, char *at_cmd)
{
	int err;
	int sms_buffer_index;

	/* Set */
	if (!prefix_match("AT+CMSS=", at_cmd)) {
		return at_cmd_custom_respond(buf, len, "ERROR\r\n");
	}

	sms_buffer_index = strtol(&(at_cmd[8]), NULL, 10);
	if (sms_buffer_index < 0 || sms_buffer_index >= SMS_BUFFER_ARRAY_SIZE) {
		return -NRF_EINVAL;
	}
	if (sms_buffers[sms_buffer_index].pdu_size == 0) {
		return -NRF_EINVAL;
	}

	err = nrf_modem_at_printf("AT+CNMI=3,2,0,0");
	if (err) {
		LOG_ERR("nrf_modem_at_printf failed on AT+CNMI=3,2,0,0, buf: %s", buf);
		return err;
	}

	/* Send AT+CMGS command to modem. */
	err = nrf_modem_at_cmd(buf, len, "AT+CMGS=%d\r%s\x1a\0",
			       sms_buffers[sms_buffer_index].pdu_size,
			       sms_buffers[sms_buffer_index].data);
	if (err) {
		LOG_ERR("nrf_modem_at_cmd failed on %s, buf: %s", __func__, buf);
		LOG_DBG("PDU size: %d", sms_buffers[sms_buffer_index].pdu_size);
		LOG_HEXDUMP_DBG(sms_buffers[sms_buffer_index].data, 340, "pdu (hex):");
		return err;
	}

	if (prefix_match("+CMGS:", buf)) {
		buf[3] = 'S';
		return 0;
	}

	return err;
}

static int at_cmd_callback_cmms(char *buf, size_t len, char *at_cmd)
{
	int err;

	/* Send to modem without buffer. */
	err = nrf_modem_at_printf(at_cmd);
	if (err) {
		if (err > 0) {
			LOG_ERR("%s failed, error_type: %d, error_value: %d", at_cmd,
				nrf_modem_at_err_type(err), nrf_modem_at_err(err));
		}
		return at_cmd_custom_respond(buf, len, "ERROR\r\n", err);
	}

	return at_cmd_custom_respond(buf, len, "OK\r\n");
}

static int at_cmd_callback_cmgw(char *buf, size_t len, char *at_cmd)
{
	char *s1 = NULL;
	char *s2 = NULL;

	/* Set */
	if (!prefix_match("AT+CMGW=", at_cmd)) {
		return at_cmd_custom_respond(buf, len, "ERROR\r\n");
	}

	s1 = strstr(at_cmd, "<CR>");
	if (s1 != NULL) {
		/* Set special char after length to use strtol to convert to string. */
		s1[0] = '\r';
		/* Move s1 to after <CR>. */
		s1 = &s1[4];
		s2 = strstr(at_cmd, "<ESC>");
	}

	if (s2 != NULL) {
		/* Find first available SMS buffer. */
		for (int i = 0; i < SMS_BUFFER_ARRAY_SIZE; i++) {
			if (sms_buffers[i].pdu_size != 0) {
				continue;
			}
			/* Store size. */
			sms_buffers[i].pdu_size = strtol(&(at_cmd[8]), NULL, 10);
			/* Verify that the buffer is large enough. */
			if ((int)(s2 - s1) + 1 > SMS_BUFFER_PDU_SIZE_MAX) {
				LOG_ERR("PDU buffer size is too small (%d > %d).",
					(int)(s2 - s1) + 1, SMS_BUFFER_PDU_SIZE_MAX);
			}
			/* Store in SMS buffer. */
			strncpy(sms_buffers[i].data, s1, (size_t)(s2 - s1));
			sms_buffers[i].data[(int)(s2 - s1)] = '\0';
			/* Make AT response. */
			return at_cmd_custom_respond(buf, len,
						     "+CMGW: %d\r\nOK\r\n", i);
		}
	}

	LOG_ERR("Could not parse command: %s", at_cmd);

	return at_cmd_custom_respond(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_cmgf(char *buf, size_t len, char *at_cmd)
{
	/* Set */
	if (prefix_match("AT+CMGF=0", at_cmd)) {
		strncpy(sca_buff, &(at_cmd[8]), SCA_SIZE_MAX);
		sca_buff[sizeof(sca_buff) - 1] = '\0';
		return at_cmd_custom_respond(buf, len, "OK\r\n");
	}

	return at_cmd_custom_respond(buf, len, "ERROR\r\n");
}

/* Initialize the library. */
int gcf_sms_init(void)
{
	for (int i = 0; i < SMS_BUFFER_ARRAY_SIZE; i++) {
		sms_buffers_clear_index(i);
	}

	return 0;
}

/* Initialize during SYS_INIT */
SYS_INIT(gcf_sms_init, APPLICATION, 1);
