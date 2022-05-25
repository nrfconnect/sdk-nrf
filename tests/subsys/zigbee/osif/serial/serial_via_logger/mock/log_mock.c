/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util.h>
#include "log_mock.h"

static uint8_t log_mock_array[20];
static int32_t log_mock_idx;

void log_mock_hexdump(uint8_t *buf, int32_t len, char *s)
{
	int32_t i;

	for (i = 0; i < len; i++) {
		if (log_mock_idx < ARRAY_SIZE(log_mock_array)) {
			log_mock_array[log_mock_idx++] = buf[i];
		}
	}
}

int32_t log_mock_get_buf_len(void)
{
	return log_mock_idx;
}

int32_t log_mock_get_buf_data(int32_t index)
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
