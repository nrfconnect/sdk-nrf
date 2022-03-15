/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stddef.h>

#include "util.h"

int util_hex2char(uint8_t x, uint8_t *c)
{
	if (x <= 9) {
		*c = x + '0';
	} else  if (x <= 15) {
		*c = x - 10 + 'a';
	} else {
		return -1;
	}

	return 0;
}

size_t util_bin2hex(const uint8_t *buf, size_t buflen,
		    uint8_t *hex, size_t hexlen)
{
	if ((hexlen + 1) < buflen * 2) {
		return 0;
	}

	for (size_t i = 0; i < buflen; i++) {
		if (util_hex2char(buf[i] >> 4, &hex[2 * i]) < 0) {
			return 0;
		}
		if (util_hex2char(buf[i] & 0xf, &hex[2 * i + 1]) < 0) {
			return 0;
		}
	}

	hex[2 * buflen] = '\0';
	return 2 * buflen;
}
