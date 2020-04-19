/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "slm_util.h"

#define PRINTABLE_ASCII(ch) (ch > 0x1f && ch < 0x7f)

/**
 * @brief Compare string ignoring case
 */
bool slm_util_casecmp(const char *str1, const char *str2)
{
	int str2_len = strlen(str2);

	if (strlen(str1) != str2_len) {
		return false;
	}

	for (int i = 0; i < str2_len; i++) {
		if (toupper((int)*(str1 + i)) != toupper((int)*(str2 + i))) {
			return false;
		}
	}

	return true;
}


/**
 * @brief Compare name of AT command ignoring case
 */
bool slm_util_cmd_casecmp(const char *cmd, const char *slm_cmd)
{
	int i;
	int slm_cmd_len = strlen(slm_cmd);

	if (strlen(cmd) < slm_cmd_len) {
		return false;
	}

	for (i = 0; i < slm_cmd_len; i++) {
		if (toupper((int)*(cmd + i)) != toupper((int)*(slm_cmd + i))) {
			return false;
		}
	}
#if defined(CONFIG_SLM_CR_LF_TERMINATION)
	if (strlen(cmd) > (slm_cmd_len + 2)) {
#else
	if (strlen(cmd) > (slm_cmd_len + 1)) {
#endif
		char ch = *(cmd + i);
		/* With parameter, SET TEST, "="; READ, "?" */
		return ((ch == '=') || (ch == '?'));
	}

	return true;
}

/**
 * @brief Detect hexdecimal data type
 */
bool slm_util_hex_check(const u8_t *data, u16_t data_len)
{
	for (int i = 0; i < data_len; i++) {
		char ch = *(data + i);

		if (!PRINTABLE_ASCII(ch) && ch != '\r' && ch != '\n') {
			return true;
		}
	}

	return false;
}

/**
 * @brief Encode hex array to hexdecimal string (ASCII text)
 */
int slm_util_htoa(const u8_t *hex, u16_t hex_len,
		char *ascii, u16_t ascii_len)
{
	if (hex == NULL || ascii == NULL) {
		return -EINVAL;
	}
	if (ascii_len < (hex_len * 2)) {
		return -EINVAL;
	}

	for (int i = 0; i < hex_len; i++) {
		sprintf(ascii + (i * 2), "%02X", *(hex + i));
	}

	return (hex_len * 2);
}

/**
 * @brief Decode hexdecimal string (ASCII text) to hex array
 */
int slm_util_atoh(const char *ascii, u16_t ascii_len,
		u8_t *hex, u16_t hex_len)
{
	char hex_str[3];

	if (hex == NULL || ascii == NULL) {
		return -EINVAL;
	}
	if ((ascii_len % 2) > 0) {
		return -EINVAL;
	}
	if (ascii_len > (hex_len * 2)) {
		return -EINVAL;
	}

	hex_str[2] = '\0';
	for (int i = 0; (i * 2) < ascii_len; i++) {
		strncpy(&hex_str[0], ascii + (i * 2), 2);
		*(hex + i) = (u8_t)strtoul(hex_str, NULL, 16);
	}

	return (ascii_len / 2);
}

/**@brief Check whether a string has valid IPv4 address or not
 */
bool check_for_ipv4(const char *address, u8_t length)
{
	int index;

	for (index = 0; index < length; index++) {
		char ch = *(address + index);

		if ((ch == '.') || (ch >= '0' && ch <= '9')) {
			continue;
		} else {
			return false;
		}
	}

	return true;
}
