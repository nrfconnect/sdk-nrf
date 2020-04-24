/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <sys/util.h>
#include "log_mock.h"

static u8_t log_mock_array[20];
static s32_t log_mock_idx;

void log_mock_hexdump(u8_t *buf, s32_t len, char *s)
{
	s32_t i;

	for (i = 0; i < len; i++) {
		if (log_mock_idx < ARRAY_SIZE(log_mock_array)) {
			log_mock_array[log_mock_idx++] = buf[i];
		}
	}
}

s32_t log_mock_get_buf_len(void)
{
	return log_mock_idx;
}

s32_t log_mock_get_buf_data(s32_t index)
{
	if (index < log_mock_idx) {
		return log_mock_array[index];
	}
	return -1;
}

void log_mock_clear(void)
{
	log_mock_idx = 0;
}
