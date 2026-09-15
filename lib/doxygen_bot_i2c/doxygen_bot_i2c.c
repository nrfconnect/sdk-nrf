/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>

#include "doxygen_bot_i2c.h"

static bool i2c_initialized;

int doxy_i2c_init(void)
{
	i2c_initialized = true;

	return 0;
}

int doxy_i2c_write(uint8_t addr, const uint8_t *data, size_t len)
{
	if (data == NULL || len == 0) {
		return -EINVAL;
	}

	if (!i2c_initialized) {
		return -ENODEV;
	}

	(void)addr;

	return 0;
}

int doxy_i2c_read(uint8_t addr, uint8_t *dst, size_t len)
{
	if (dst == NULL || len == 0) {
		return -EINVAL;
	}

	if (!i2c_initialized) {
		return -ENODEV;
	}

	(void)addr;

	return 0;
}

int doxy_i2c_recover(void)
{
	if (!i2c_initialized) {
		return -ENODEV;
	}

	return 0;
}
