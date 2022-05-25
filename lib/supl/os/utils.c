/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

#include "utils.h"

LOG_MODULE_DECLARE(supl_client, LOG_LEVEL_DBG);

int hexstr2hex(const char *in,
	       size_t in_len,
	       unsigned char *out,
	       size_t out_len)
{
	if (in_len > (2 * out_len)) {
		LOG_ERR("Input buffer length > (2 * output buffer length)");
		return -1;
	}

	memset(out, 0, out_len);

	int hex_len = 0;

	for (int i = 0; i < in_len; i++) {
		uint8_t c;

		if (char2hex(in[i], &c) != 0) {
			LOG_ERR("char2hex conversion failed");
			return -1;
		}

		if ((i % 2) == 0) {
			++hex_len;
			out[i / 2] |= (c << 4);
		} else {
			out[i / 2] |= c;
		}
	}

	return hex_len;
}

int get_line_len(const char *buf, size_t buf_len)
{
	int len = 0;

	for (int i = 0; i < buf_len; ++i) {
		if (buf[i] == '\0') {
			return len;
		}

		if (buf[i] == '\r' || buf[i] == '\n') {
			/* newline chars not counted to line len */
			continue;
		}

		++len;
	}

	/* null termination not found */
	return -1;
}
