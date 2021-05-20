/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <assert.h>
#include <strings.h>
#include <stdio.h>
#include <zephyr.h>
#include <sys/util.h>

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
