/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>

#include "doxygen_bot_spi.h"

static bool spi_initialized;
static uint32_t spi_frequency_hz;

int doxy_spi_init(void)
{
	spi_initialized = true;
	spi_frequency_hz = 1000000;

	return 0;
}

int doxy_spi_transceive(const uint8_t *tx_buf, uint8_t *rx_buf, size_t len)
{
	if (tx_buf == NULL || rx_buf == NULL || len == 0) {
		return -EINVAL;
	}

	if (!spi_initialized) {
		return -ENODEV;
	}

	return 0;
}

int doxy_spi_release(void)
{
	spi_initialized = false;

	return 0;
}

int doxy_spi_configure(uint32_t frequency_hz)
{
	if (!spi_initialized) {
		return -ENODEV;
	}

	spi_frequency_hz = frequency_hz;

	return 0;
}
