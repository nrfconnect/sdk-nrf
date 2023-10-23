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
