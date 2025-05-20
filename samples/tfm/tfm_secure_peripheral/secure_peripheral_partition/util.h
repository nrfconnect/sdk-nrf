/*
 * Copyright (c) 2022 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <stddef.h>

int util_hex2char(uint8_t x, uint8_t *c);

size_t util_bin2hex(const uint8_t *buf, size_t buflen,
		    uint8_t *hex, size_t hexlen);

static inline void util_put_le16(uint16_t val, uint8_t dst[2])
{
	dst[0] = val;
	dst[1] = val >> 8;
}

static inline void util_put_le32(uint32_t val, uint8_t dst[4])
{
	util_put_le16(val, dst);
	util_put_le16(val >> 16, &dst[2]);
}
