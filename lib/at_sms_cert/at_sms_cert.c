/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <nrf_errno.h>
#include <nrf_modem_at.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(at_sms_cert, CONFIG_AT_SMS_CERT_LOG_LEVEL);

/* Buffer size for SMS messages. */
#define SMS_BUFFER_LIST_SIZE (3)
#define SMS_BUFFER_MAX_PDU_SIZE (165)

/* Buffer size for Service Center Address. */
#define SCA_BUFFER_SIZE (16)

/* Buffer for SMS messages. */
struct sms_buffer {
	char data[SMS_BUFFER_MAX_PDU_SIZE];
	size_t pdu_size;
};

/* Buffer list for SMS messages. */
static struct sms_buffer sms_buffer_list[SMS_BUFFER_LIST_SIZE];

/* Buffer for SCA. */
static char sca_buff[SCA_BUFFER_SIZE + 1];

/* AT filter function declarations. */
static int at_cmd_callback_cpms(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_csms(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_csca(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cscs(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cmgd(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cmss(char *buf, size_t len, char *at_cmd);
static int at_cmd_callback_cmgw(char *buf, size_t len, char *at_cmd);

/* AT filter lits
 * Including all comands the filter should check for and function ptr
 * to functions to be called on detection.
 */
static const struct nrf_modem_at_cmd_filter at_cmd_filter_list[] = {
	{"AT+CPMS", &at_cmd_callback_cpms},
	{"AT+CSMS", &at_cmd_callback_csms},
	{"AT+CSCA", &at_cmd_callback_csca},
	{"AT+CSCS", &at_cmd_callback_cscs},
	{"AT+CMGD", &at_cmd_callback_cmgd},
	{"AT+CMSS", &at_cmd_callback_cmss},
	{"AT+CMGW", &at_cmd_callback_cmgw}
};

/* Match string. */
static int match_string(char *str, const char *at_cmd)
{
	return (strncmp(str, at_cmd, strlen(str)) == 0);
}

/* Fill response buffer without overflowing the buffer. */
static int response_buffer_fill(char *buf, size_t len, const char *content, ...)
{
	va_list args;

	if (buf == NULL) {
		LOG_ERR("%s called with NULL buffer", __func__);
		return -NRF_EFAULT;
	}

	va_start(args, content);
	if (vsnprintf(buf, len, content, args) > len) {
		LOG_ERR("The buffer passed to %s is too small", __func__);
		return -NRF_E2BIG;
	}
	va_end(args);

	return 0;
}

/*
 * AT Filter callback functions.
 */

static int at_cmd_callback_cpms(char *buf, size_t len, char *at_cmd)
{
	int sms_storage_used = 0;

	/* Test */
	if (match_string("AT+CPMS=?", at_cmd)) {
		return response_buffer_fill(buf, len,
				"\r\n+CPMS: (\"TA\"),(\"TA\")\r\nOK\r\n");
	}

	/* Set */
	if (match_string("AT+CPMS=\"TA\",\"TA\"", at_cmd)) {

		/* Find number of storage in use. */
		for (int i = 0; i < SMS_BUFFER_LIST_SIZE; i++) {
			if (sms_buffer_list[i].pdu_size != 0) {
				sms_storage_used++;
			}
		}

		return response_buffer_fill(buf, len, "\r\n+CPMS: %d,%d,%d,%d\r\nOK\r\n",
				sms_storage_used, SMS_BUFFER_LIST_SIZE, sms_storage_used,
				SMS_BUFFER_LIST_SIZE);
	}
	if (match_string("AT+CPMS=\"SM\",\"SM\",\"MT\"", at_cmd)) {

		/* Find number of storage in use. */
		for (int i = 0; i < SMS_BUFFER_LIST_SIZE; i++) {
			if (sms_buffer_list[i].pdu_size != 0) {
				sms_storage_used++;
			}
		}

		return response_buffer_fill(buf, len, "\r\n+CPMS: \"SM\",%d,%d,"
				"\"SM\",%d,%d,\"MT\",%d,%d\r\nOK\r\n",
				sms_storage_used, SMS_BUFFER_LIST_SIZE, sms_storage_used,
				SMS_BUFFER_LIST_SIZE, sms_storage_used, SMS_BUFFER_LIST_SIZE);
	}

	/* Read */
	if (match_string("AT+CPMS?", at_cmd)) {

		/* Find number of storage in use. */
		for (int i = 0; i < SMS_BUFFER_LIST_SIZE; i++) {
			if (sms_buffer_list[i].pdu_size != 0) {
				sms_storage_used++;
			}
		}

		return response_buffer_fill(buf, len,
				"\r\n+CPMS: \"TA\",%d,%d,\"TA\",%d,%d\r\nOK\r\n",
				sms_storage_used, SMS_BUFFER_LIST_SIZE,
				sms_storage_used, SMS_BUFFER_LIST_SIZE);
	}

	return response_buffer_fill(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_csms(char *buf, size_t len, char *at_cmd)
{
	/* Test */
	if (match_string("AT+CSMS=?", at_cmd)) {
		return response_buffer_fill(buf, len, "\r\n+CSMS: 0\r\nOK\r\n");
	}

	/* Set */
	if (match_string("AT+CSMS=0", at_cmd)) {
		return response_buffer_fill(buf, len, "\r\n+CSMS: 1,1,0\r\nOK\r\n");
	} else if (match_string("AT+CSMS=1", at_cmd)) {
		return response_buffer_fill(buf, len, "\r\n+CSMS: 0,0,0\r\nOK\r\n");
	}

	/* Read */
	if (match_string("AT+CSMS?", at_cmd)) {
		return response_buffer_fill(buf, len, "\r\n+CSMS: 0,1,1,0\r\nOK\r\n");
	}

	return response_buffer_fill(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_csca(char *buf, size_t len, char *at_cmd)
{
	/* Set */
	if (match_string("AT+CSCA=", at_cmd)) {
		strncpy(sca_buff, &(at_cmd[8]), SCA_BUFFER_SIZE);
		sca_buff[sizeof(sca_buff) - 1] = '\0';
		return response_buffer_fill(buf, len, "\r\n+CSMS: OK\r\nOK\r\n");
	}

	/* Read */
	if (match_string("AT+CSCA?", at_cmd)) {
		return response_buffer_fill(buf, len, "\r\n+CSMS: %s\r\nOK\r\n", sca_buff);
	}

	return response_buffer_fill(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_cscs(char *buf, size_t len, char *at_cmd)
{
	/* Set */
	if (match_string("AT+CSCS=", at_cmd)) {
		strncpy(sca_buff, &(at_cmd[8]), SCA_BUFFER_SIZE);
		sca_buff[sizeof(sca_buff) - 1] = '\0';
		return response_buffer_fill(buf, len, "\r\nOK\r\n");
	}

	return response_buffer_fill(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_cmgd(char *buf, size_t len, char *at_cmd)
{
	/* Test */
	if (match_string("AT+CMGD=?", at_cmd)) {
		return response_buffer_fill(buf, len, "\r\n+CMGD: (1-3)\r\nOK\r\n");
	}

	/* Set */
	if (match_string("AT+CMGD=", at_cmd)) {
		int index = strtol(&(at_cmd[8]), NULL, 10) - 1;

		if (index >= 0 && index < SMS_BUFFER_LIST_SIZE) {
			sms_buffer_list[index].pdu_size = 0;
			return response_buffer_fill(buf, len, "\r\nOK\r\n");
		}

	}

	return response_buffer_fill(buf, len, "ERROR\r\n");
}

static int at_cmd_callback_cmss(char *buf, size_t len, char *at_cmd)
{
	int err;
	int sms_buffer_index;

	/* Set */
	if (!match_string("AT+CMSS=", at_cmd)) {
		return response_buffer_fill(buf, len, "ERROR\r\n");
	}

	sms_buffer_index = strtol(&(at_cmd[8]), NULL, 10) - 1;
	if (sms_buffer_index < 0 || sms_buffer_index >= SMS_BUFFER_LIST_SIZE) {
		return -NRF_EINVAL;
	}

	if (sms_buffer_list[sms_buffer_index].pdu_size == 0) {
		return -NRF_EINVAL;
	}

	LOG_DBG("Sending to modem: AT+CMGS=%d<CR>\n%s",
			sms_buffer_list[sms_buffer_index].pdu_size,
			sms_buffer_list[sms_buffer_index].data);
	/* Send AT+CMGS command to modem. */
	err = nrf_modem_at_cmd(buf, len, "AT+CMGS=%d\r%s\x1a",
			sms_buffer_list[sms_buffer_index].pdu_size,
			sms_buffer_list[sms_buffer_index].data);
	if (err) {
		LOG_ERR("nrf_modem_at_cmd failed on %s, buf: %s", __func__, buf);
		return err;
	}

	if (match_string("+CMGS:", buf)) {
		buf[3] = 'S';
		return 0;
	}

	return err;
}

static int at_cmd_callback_cmgw(char *buf, size_t len, char *at_cmd)
{
	char *s1 = NULL;
	char *s2 = NULL;

	/* Set */
	if (!match_string("AT+CMGW=", at_cmd)) {
		return response_buffer_fill(buf, len, "ERROR\r\n");
	}

	s1 = strstr(at_cmd, "\r");
	if (s1 != NULL) {
		/* Move s1 to after <CR>. */
		s1 = &s1[1];
		s2 = strstr(at_cmd, "\x1a");
	}

	if (s2 != NULL) {
		/* Find first available SMS buffer. */
		for (int i = 0; i < SMS_BUFFER_LIST_SIZE; i++) {
			if (sms_buffer_list[i].pdu_size != 0) {
				continue;
			}
			/* Store size. */
			sms_buffer_list[i].pdu_size = strtol(&(at_cmd[8]), NULL, 10);
			/* Store in SMS buffer. */
			strncpy(sms_buffer_list[i].data, s1, (size_t) (s2 - s1));
			/* Make AT response. */
			return response_buffer_fill(buf, len, "\r\n+CMGW: %d\r\nOK\r\n", i + 1);
		}
	}

	return response_buffer_fill(buf, len, "ERROR\r\n");
}

/* Initialize AT filter */
int at_sms_cert_init(const struct device *unused)
{
	int err;

	(void)unused;

	err = nrf_modem_at_cmd_filter_set((struct nrf_modem_at_cmd_filter *)at_cmd_filter_list,
					  ARRAY_SIZE(at_cmd_filter_list));
	if (err) {
		return err;
	}

	for (int i = 0; i < SMS_BUFFER_LIST_SIZE; i++) {
		sms_buffer_list[i].pdu_size = 0;
	}

	return 0;
}

/* Deinitialize AT filter */
int at_sms_cert_deinit(void)
{
	int err;

	err = nrf_modem_at_cmd_filter_set(NULL, 0);
	if (err) {
		return err;
	}

	return 0;
}

#if defined(CONFIG_AT_SMS_CERT_SYS_INIT)
SYS_INIT(at_sms_cert_init, APPLICATION, 0);
#endif
