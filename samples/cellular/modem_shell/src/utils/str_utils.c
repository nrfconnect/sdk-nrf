/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <nrf_modem_gnss.h>

#include "str_utils.h"
#include "mosh_print.h"

char *shell_command_str_from_argv(
	size_t argc,
	char **argv,
	const char *cmd_prefix,
	char *out_buf,
	uint16_t out_buf_len)
{
	int i, total_len = 0, arg_len = 0;

	if (cmd_prefix != NULL) {
		strncpy(out_buf, cmd_prefix, out_buf_len);
		total_len = strlen(cmd_prefix);
	}

	for (i = 0; i < argc; i++) {
		arg_len = strlen(argv[i]);
		if (total_len + arg_len > out_buf_len) {
			mosh_error("Converted shell command cut due to too short output buffer");
			break;
		}
		if (i == (argc - 1)) {
			/* Do not put space to last one */
			sprintf(out_buf + total_len, "%s", argv[i]);
		} else {
			sprintf(out_buf + total_len, "%s ", argv[i]);
		}
		total_len = strlen(out_buf);
	}

	return out_buf;
}

int str_hex_to_bytes(char *str, uint32_t str_length, uint8_t *buf, uint16_t buf_length)
{
	/* Remove any spaces from the input string */
	uint32_t str_length_no_space = 0;

	for (int i = 0; i < str_length; i++) {
		if (str[i] != ' ') {
			str[str_length_no_space] = str[i];
			str_length_no_space++;
		}
	}

	buf_length = hex2bin(str, str_length_no_space, buf, buf_length);

	return buf_length;
}

char *mosh_strdup(const char *str)
{
	size_t len;
	char *newstr;

	if (!str) {
		return (char *)NULL;
	}

	len = strlen(str) + 1;
	newstr = malloc(len);
	if (!newstr) {
		return (char *)NULL;
	}

	memcpy(newstr, str, len);
	return newstr;
}

void agnss_data_flags_str_get(char *flags_string, uint32_t data_flags)
{
	size_t len;

	*flags_string = '\0';

	if (data_flags & NRF_MODEM_GNSS_AGNSS_GPS_UTC_REQUEST) {
		(void)strcat(flags_string, "utc | ");
	}
	if (data_flags & NRF_MODEM_GNSS_AGNSS_KLOBUCHAR_REQUEST) {
		(void)strcat(flags_string, "klob | ");
	}
	if (data_flags & NRF_MODEM_GNSS_AGNSS_NEQUICK_REQUEST) {
		(void)strcat(flags_string, "neq | ");
	}
	if (data_flags & NRF_MODEM_GNSS_AGNSS_GPS_SYS_TIME_AND_SV_TOW_REQUEST) {
		(void)strcat(flags_string, "time | ");
	}
	if (data_flags & NRF_MODEM_GNSS_AGNSS_POSITION_REQUEST) {
		(void)strcat(flags_string, "pos | ");
	}
	if (data_flags & NRF_MODEM_GNSS_AGNSS_INTEGRITY_REQUEST) {
		(void)strcat(flags_string, "int | ");
	}

	len = strlen(flags_string);
	if (len == 0) {
		(void)strcpy(flags_string, "none");
	} else {
		flags_string[len - 3] = '\0';
	}
}

const char *gnss_system_str_get(uint8_t system_id)
{
	switch (system_id) {
	case NRF_MODEM_GNSS_SYSTEM_INVALID:
		return "invalid";

	case NRF_MODEM_GNSS_SYSTEM_GPS:
		return "GPS";

	case NRF_MODEM_GNSS_SYSTEM_QZSS:
		return "QZSS";

	default:
		return "unknown";
	}
}
